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
  pubSock(ctx, ZMQ_PUB),
  subSock(ctx, ZMQ_SUB),
  gossipPushSock(ctx, ZMQ_PUSH),
  gossipPullSock(ctx, ZMQ_PULL),
  db()
{
    if (!db.init_db(cfg.dbPath)) {
        cerr << "[Node " << cfg.nodeId << "] failed to init db: " << cfg.dbPath << "\n";
    }

    string repAddr = "tcp://" + cfg.host + ":" + to_string(cfg.clientPort);
    repSock.bind(repAddr);

    string pubAddr = "tcp://" + cfg.host + ":" + to_string(cfg.pubPort);
    pubSock.bind(pubAddr);

    subSock.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    for (const auto& ep : cfg.peerGossipPullEndpoints) {
        gossipPushSock.connect(ep);
    }

    {
        std::string addr = "tcp://" + cfg.host + ":" + std::to_string(cfg.gossipPullPort);
        gossipPullSock.bind(addr);
    }


    running = false;
}

Node::~Node() {
    stop();
    repSock.close();
    pubSock.close();
    subSock.close();
    gossipPushSock.close();
    gossipPullSock.close();
    ctx.close();
}

void Node::addPeerPubEndpoint(const string& ep) {
    subSock.connect(ep);
}

void Node::start() {
    if (running) return;
    running = true;
    loopThread = thread(&Node::run_loop, this);
}

void Node::stop() {
    if (!running) return;
    running = false;
    if (loopThread.joinable()) loopThread.join();
}

int Node::shard_for_list(const string& listId) const {
    return Util::getHash(listId) % cfg.numShards;
}

void Node::run_loop() {
    uint64_t nextGossipTs = next_gossip_ts();

    zmq::pollitem_t items[] = {
        { static_cast<void*>(repSock), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(subSock), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(gossipPullSock), 0, ZMQ_POLLIN, 0 }
    };

    while (running) {
        zmq::poll(items, 3, chrono::milliseconds(100));

        if (items[0].revents & ZMQ_POLLIN) {
            handle_client_frame();
        }

        if (items[1].revents & ZMQ_POLLIN) {
            handle_sub_frames();
        }

        if (items[2].revents & ZMQ_POLLIN) {
            handle_gossip_frame();
        }

        if (Util::now_ms() >= nextGossipTs) {
            perform_gossip();
            nextGossipTs = next_gossip_ts();
        }
    }
}

void Node::handle_client_frame() {
    zmq::message_t frame;
    repSock.recv(frame);

    Message m = Message::from_zmq(frame);

    int s = shard_for_list(m.lists[0].getUid());
    if (s != cfg.shardId) {
        string err = "WRONG_SHARD"; // TODO: rethink error handling
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
    broadcast_message(m);

    Message resp = Message::list_response(
        true,
        cfg.nodeId,
        Util::now_ms(),
        m.lists[0]
    );

    repSock.send(resp.to_zmq(), zmq::send_flags::none);

}

void Node::handle_sub_frames() {
    zmq::message_t topicFrame;
    zmq::message_t payloadFrame;
    subSock.recv(topicFrame);
    subSock.recv(payloadFrame);

    string topic(static_cast<char*>(topicFrame.data()), topicFrame.size());
    int shard = -1;
    if (topic.rfind("shard.", 0) == 0) {
        try { shard = stoi(topic.substr(6)); }
        catch (...) { shard = -1; }
    }
    if (shard < 0) return;
    if (shard != cfg.shardId) return;

    message::Message m = message::Message::from_zmq(payloadFrame);
    apply_message(m);
}

void Node::handle_gossip_frame() {
    zmq::message_t gf;
    gossipPullSock.recv(gf);
    message::Message gm = message::Message::from_zmq(gf);
    if (gm.op == OpType::GOSSIP_LISTS) {
        apply_message(gm);
    }
}

void Node::apply_message(const message::Message& m) {
    switch (m.op) {
        case OpType::ENSURE_LIST: {
            ShoppingList list = m.lists[0];
            db.write(m.lists[0]);
            break;
        }
        case OpType::DELETE_LIST: {
            db.delete_list(m.lists[0].getUid());
            break;
        }
        case OpType::GOSSIP_LISTS: {
            for (auto& l : m.lists)  db.write(l);
            break;
        }
        default:
            break;
    }
}

void Node::broadcast_message(const message::Message& m) {
    string topic = "shard." + to_string(cfg.shardId);
    zmq::message_t topicFrame(topic.size());
    memcpy(topicFrame.data(), topic.data(), topic.size());

    zmq::message_t payload = m.to_zmq();

    pubSock.send(topicFrame, zmq::send_flags::sndmore);
    pubSock.send(payload, zmq::send_flags::none);
}

void Node::perform_gossip() {
    std::vector<ShoppingList> lists;
    lists = db.read_all();


    message::Message m =
        message::Message::gossip_lists(cfg.nodeId, Util::now_ms(), lists);

    if (!cfg.peerGossipPullEndpoints.empty()) {
        zmq::message_t f = m.to_zmq();
        gossipPushSock.send(f, zmq::send_flags::none);
    }
}

uint64_t Node::next_gossip_ts() const {
    int jitter = Util::rand_int(-cfg.gossipIntervalMs / 3, cfg.gossipIntervalMs / 3);
    return Util::now_ms() + cfg.gossipIntervalMs + jitter;
}
