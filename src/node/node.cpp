#include "node.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

#include "util.cpp"

using namespace message;
using namespace std;

Node::Node(const NodeConfig& c)
: cfg(c),
  ctx(1),
  repSock(ctx, ZMQ_REP),
  gossipPushSock(ctx, ZMQ_PUSH),
  gossipPullSock(ctx, ZMQ_PULL),
  discoveryPushSock(ctx, ZMQ_PUSH),
  discoveryPullSock(ctx, ZMQ_PULL),
  db()
{
    if (!db.init_db(cfg.dbPath)) {
        cerr << "[Node " << cfg.nodeId << "] failed to init db: " << cfg.dbPath << "\n";
    }

    knownNodes[cfg.nodeId] = NodeInfo{
        cfg.nodeId,
        cfg.host,
        cfg.shardId,
        cfg.clientPort,
        cfg.gossipPullPort,
        cfg.discoveryPullPort,
        Util::now_ms()
    };

    string repAddr = "tcp://" + cfg.host + ":" + to_string(cfg.clientPort);
    repSock.bind(repAddr);

    {
        std::string addr = "tcp://" + cfg.host + ":" + std::to_string(cfg.gossipPullPort);
        gossipPullSock.bind(addr);
    }
    
    {
        std::string addr = "tcp://" + cfg.host + ":" + std::to_string(cfg.discoveryPullPort);
        discoveryPullSock.bind(addr);
    }

    running = false;
}

Node::~Node() {
    stop();
    repSock.close();
    gossipPushSock.close();
    gossipPullSock.close();
    discoveryPullSock.close();
    discoveryPushSock.close();
    ctx.close();
}

void Node::start() {
    if (running) return;
    running = true;
    loopThread = thread(&Node::run_loop, this);
}

void Node::stop() {
    if (!running) return;
    running = false;
    ctx.shutdown();
    if (loopThread.joinable())
        loopThread.join();
}

int Node::shard_for_list(const string& listId) const {
    return Util::getHash(listId) % cfg.numShards;
}

void Node::run_loop() {
    uint64_t nextStateGossipTs = next_gossip_ts(cfg.gossipIntervalMs);
    uint64_t nextDiscoveryGossipTs = next_gossip_ts(cfg.discoveryIntervalMs);

    zmq::pollitem_t items[] = {
        { static_cast<void*>(repSock), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(gossipPullSock), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(discoveryPullSock), 0, ZMQ_POLLIN, 0 }
    };

    update_known_nodes(cfg.initialPeers);
    for (auto& [nodeId, info] : knownNodes) {
        info.lastSeenTs = Util::now_ms();
    }

    try {
        
        while (running) {

            zmq::poll(items, 3, chrono::milliseconds(100));

            if (items[0].revents & ZMQ_POLLIN)
                handle_client_frame();

            if (items[1].revents & ZMQ_POLLIN)
                handle_gossip_frame();

            if (items[2].revents & ZMQ_POLLIN)
                handle_discovery_frame();

            if (Util::now_ms() >= nextStateGossipTs) {
                perform_shard_gossip();
                nextStateGossipTs = next_gossip_ts(cfg.gossipIntervalMs);
            }

            if (Util::now_ms() >= nextDiscoveryGossipTs) {
                evict_dead_nodes();
                perform_discovery_gossip();
                nextDiscoveryGossipTs = next_gossip_ts(cfg.discoveryIntervalMs);
            }

        }


    } catch (const zmq::error_t& e) {
        if (e.num() == ETERM || e.num() == EAGAIN) {
            return;
        }
    }
}

void Node::handle_client_frame() {
    zmq::message_t frame;
    repSock.recv(frame, zmq::recv_flags::none);

    Message m = Message::from_zmq(frame);

    if (m.op == OpType::GET_NODES) {
        vector<NodeInfo> nodes;
        for (auto& [_, n] : knownNodes)
            nodes.push_back(n);   
        Message resp = Message::nodes_response(
            cfg.nodeId,
            Util::now_ms(),
            nodes
        );

        repSock.send(resp.to_zmq(), zmq::send_flags::none);
        return;
    }

    int s = shard_for_list(m.lists[0].getUid());
    if (s != cfg.shardId) {
        string err = "WRONG_SHARD";
        zmq::message_t errm(err.size());
        memcpy(errm.data(), err.data(), err.size());
        repSock.send(errm, zmq::send_flags::none);
        return;
    }

    if (m.op == OpType::GET_LIST) {
        optional<ShoppingList> opt;
        string listUid = m.lists[0].getUid();
        opt = db.read(listUid);

        Message resp = opt.has_value() ?
            Message::list_response(true, cfg.nodeId, Util::now_ms(), *opt) :
            Message::list_response(false, cfg.nodeId, Util::now_ms(), ShoppingList(listUid, ""));
        zmq::message_t out = resp.to_zmq();
        repSock.send(out, zmq::send_flags::none);
        return;
    }

    m.origin = cfg.nodeId;
    m.ts = Util::now_ms();

    apply_message(m);
    eager_fanout();

    Message resp = Message::list_response(
        m.op == OpType::DELETE_LIST ? false : true,
        cfg.nodeId,
        Util::now_ms(),
        m.lists[0]
    );

    repSock.send(resp.to_zmq(), zmq::send_flags::none);
}

void Node::handle_gossip_frame() {
    zmq::message_t gf;
    gossipPullSock.recv(gf, zmq::recv_flags::none);
    Message gm = Message::from_zmq(gf);

    if (gm.op == OpType::GOSSIP_LISTS)
        apply_message(gm);
}

void Node::apply_message(const Message& m) {
    switch (m.op) {
        case OpType::ENSURE_LIST: {
            ShoppingList incomingList = m.lists[0];
            ShoppingList existingList = db.read(incomingList.getUid()).value_or(ShoppingList(incomingList.getUid(), ""));
            existingList.merge(incomingList);
            db.write(existingList);
            break;
        }
        case OpType::DELETE_LIST: {
            db.delete_list(m.lists[0].getUid());
            break;
        }
        case OpType::GOSSIP_LISTS: {
            for (auto& incomingList : m.lists) {
                ShoppingList existingList = db.read(incomingList.getUid()).value_or(ShoppingList(incomingList.getUid(), ""));
                existingList.merge(incomingList);
                db.write(existingList);
            }
            break;
        }
        default:
            break;
    }
}

void Node::eager_fanout() {
    for (int i = 0; i < 3; i++)
        perform_shard_gossip();
}

void Node::perform_shard_gossip() {
    vector<ShoppingList> lists = db.read_all();

    Message m = Message::gossip_lists(
        cfg.nodeId,
        Util::now_ms(),
        lists
    );

    try {
        gossipPushSock.send(m.to_zmq(), zmq::send_flags::dontwait);
    } catch (const zmq::error_t& e) {
        if (e.num() != EAGAIN) {
            return;
        }
    }
}

void Node::perform_discovery_gossip() {
    knownNodes[cfg.nodeId].lastSeenTs = Util::now_ms();
    vector<NodeInfo> nodes;
    for (auto& [_, n] : knownNodes)
        nodes.push_back(n);

    Message m = Message::gossip_nodes(
        cfg.nodeId,
        Util::now_ms(),
        nodes
    );

    try {
        discoveryPushSock.send(m.to_zmq(), zmq::send_flags::dontwait);
    } catch (const zmq::error_t& e) {
        if (e.num() != EAGAIN) {
            return;
        }
    }
}

void Node::handle_discovery_frame() {
    zmq::message_t gf;
    discoveryPullSock.recv(gf, zmq::recv_flags::none);
    Message gm = Message::from_zmq(gf);

    if (gm.op != OpType::GOSSIP_NODES)
        return;

    update_known_nodes(gm.nodes);
}

void Node::update_known_nodes(const std::vector<message::NodeInfo>& nodes) {
    for (auto& n : nodes) {

        if (n.nodeId == cfg.nodeId)
            continue;

        auto it = knownNodes.find(n.nodeId);
        bool isNew = (it == knownNodes.end());

        if (!isNew && it->second.lastSeenTs > n.lastSeenTs)
            continue;

        if (isNew) {
            knownNodes[n.nodeId] = n;

            if (connectedDiscovery.insert(n.nodeId).second) {
                std::string ep =
                    "tcp://" + n.host + ":" + std::to_string(n.discoveryPullPort);
                discoveryPushSock.connect(ep);
            }

            if (n.shardId == cfg.shardId &&
                connectedShard.insert(n.nodeId).second) {
                std::string ep =
                    "tcp://" + n.host + ":" + std::to_string(n.gossipPullPort);
                gossipPushSock.connect(ep);
            }
        } else {
            it->second = n;
        }

        // Only update lastSeenTs to the gossiped value, not to now
        knownNodes[n.nodeId].lastSeenTs = n.lastSeenTs;
    }
}

uint64_t Node::next_gossip_ts(uint64_t interval) const {
    int i = static_cast<int>(interval);
    int jitter = Util::rand_int(-i / 3, i / 3);
    return Util::now_ms() + interval + jitter;
}

void Node::evict_dead_nodes() {
    uint64_t now = Util::now_ms();

    for (auto it = knownNodes.begin(); it != knownNodes.end(); ) {
        const auto& nodeId = it->first;
        const auto& info = it->second;

        if (nodeId == cfg.nodeId) {
            ++it;
            continue;
        }

        if (now - info.lastSeenTs <= cfg.discoveryTimeoutMs) {
            ++it;
            continue;
        }

        if (connectedDiscovery.erase(nodeId)) {
            std::string ep =
                "tcp://" + info.host + ":" + std::to_string(info.discoveryPullPort);
            discoveryPushSock.disconnect(ep);
        }

        if (info.shardId == cfg.shardId && connectedShard.erase(nodeId)) {
            std::string ep =
                "tcp://" + info.host + ":" + std::to_string(info.gossipPullPort);
            gossipPushSock.disconnect(ep);
        }

        it = knownNodes.erase(it);
    }
}

