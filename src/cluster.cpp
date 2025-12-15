#include "node/node.hpp"
#include "message/message.hpp"
#include "util.cpp"

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <zmq.hpp>

using namespace std;
using namespace message;

static void sleep_ms(int ms) {
    this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct RunningNode {
    unique_ptr<Node> node;   // null if stopped
    NodeConfig cfg;
    bool alive;
};

NodeConfig make_config(
    int idx,
    int shardId,
    int numShards,
    int baseClientPort,
    int baseGossipPullPort,
    int baseDiscoveryPullPort
) {
    NodeConfig c;
    c.nodeId = "N" + to_string(idx);
    c.host = "127.0.0.1";
    c.clientPort = baseClientPort + idx;
    c.gossipPullPort = baseGossipPullPort + idx;
    c.discoveryPullPort = baseDiscoveryPullPort + idx;
    c.numShards = numShards;
    c.shardId = shardId;
    c.gossipIntervalMs = 500;
    c.discoveryIntervalMs = 1000;
    c.discoveryTimeoutMs = 10000;
    c.dbPath = to_string(idx) + "test.db";
    return c;
}

int main() {
    const int numShards = 2;
    const int baseClientPort = 5000;
    const int baseGossipPullPort = 7000;
    const int baseDiscoveryPullPort = 8000;

    vector<RunningNode> nodes;
    int nextNodeIdx = 0;

    // ----------------------------
    // Bootstrap initial cluster
    // ----------------------------
    
    // 1. Pre-calculate the info for the bootstrap nodes
    //    so they can reference each other immediately.
    vector<NodeInfo> bootstrapPeers;
    for (int i = 0; i < numShards; i++) {
        bootstrapPeers.push_back({
            "N" + to_string(i),         // nodeId matches make_config logic
            "127.0.0.1",                // host
            i,                          // shardId
            baseClientPort + i,         // clientPort
            baseGossipPullPort + i,     // gossipPullPort
            baseDiscoveryPullPort + i,  // discoveryPullPort
            0                           // lastSeenTs (0 or now is fine)
        });
    }

    // 2. Create and start the nodes
    for (int shard = 0; shard < numShards; shard++) {
        // We use the same index logic as the pre-calc above
        int currentIdx = nextNodeIdx++; 
        
        NodeConfig cfg = make_config(
            currentIdx,
            shard,
            numShards,
            baseClientPort,
            baseGossipPullPort,
            baseDiscoveryPullPort
        );

        // Inject knowledge of all OTHER bootstrap peers
        for (const auto& peer : bootstrapPeers) {
            if (peer.nodeId != cfg.nodeId) {
                cfg.initialPeers.push_back(peer);
            }
        }

        auto node = make_unique<Node>(cfg);
        node->start();
        nodes.push_back({ std::move(node), cfg, true });
        
        cout << "[BOOTSTRAP] Started " << cfg.nodeId 
             << " with " << cfg.initialPeers.size() << " initial peers.\n";
    }

    cout << "[CLUSTER] Ready\n";
    cout << "Commands:\n";
    cout << "  add <shardId>\n";
    cout << "  stop <nodeId>\n";
    cout << "  start <nodeId>\n";
    cout << "  remove <nodeId>\n";
    cout << "  list\n";
    cout << "  peers <nodeId>\n";
    cout << "  quit\n";

    zmq::context_t cliCtx(1);

    // ----------------------------
    // Interactive loop
    // ----------------------------
    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;

        stringstream ss(line);
        string cmd;
        ss >> cmd;

        // ---------------- add ----------------
        if (cmd == "add") {
            int shardId;
            ss >> shardId;

            if (ss.fail() || shardId < 0 || shardId >= numShards) {
                cout << "[ERROR] Usage: add <shardId>\n";
                continue;
            }

            // Find a bootstrap peer to help the new node join
            NodeInfo bootstrapInfo;
            bool found = false;

            for (auto& rn : nodes) {
                if (rn.alive && rn.cfg.shardId == shardId) {
                    // Copy info safely
                    bootstrapInfo = {
                        rn.cfg.nodeId,
                        rn.cfg.host,
                        rn.cfg.shardId,
                        rn.cfg.clientPort,
                        rn.cfg.gossipPullPort,
                        rn.cfg.discoveryPullPort,
                        0
                    };
                    found = true;
                    break;
                }
            }

            if (!found) {
                cout << "[ERROR] No alive node in shard "
                     << shardId << "\n";
                continue;
            }

            NodeConfig cfg = make_config(
                nextNodeIdx++,
                shardId,
                numShards,
                baseClientPort,
                baseGossipPullPort,
                baseDiscoveryPullPort
            );

            // Add the found peer to initial peers
            cfg.initialPeers.push_back(bootstrapInfo);

            auto node = make_unique<Node>(cfg);
            node->start();
            nodes.push_back({ std::move(node), cfg, true });

            cout << "[CLUSTER] Added node " << cfg.nodeId
                 << " to shard " << shardId << "\n";
        }

        // ---------------- stop ----------------
        else if (cmd == "stop") {
            string nodeId;
            ss >> nodeId;

            for (auto& rn : nodes) {
                if (rn.cfg.nodeId == nodeId && rn.alive) {
                    rn.node->stop();
                    rn.node.reset();
                    rn.alive = false;
                    cout << "[CLUSTER] Stopped node " << nodeId << "\n";
                    goto done;
                }
            }
            cout << "[ERROR] Node not found or already stopped\n";
        done:;
        }

        // ---------------- start ----------------
        else if (cmd == "start") {
            string nodeId;
            ss >> nodeId;

            for (auto& rn : nodes) {
                if (rn.cfg.nodeId == nodeId && !rn.alive) {
                    rn.node = make_unique<Node>(rn.cfg);
                    rn.node->start();
                    rn.alive = true;
                    cout << "[CLUSTER] Started node " << nodeId << "\n";
                    goto done2;
                }
            }
            cout << "[ERROR] Node not found or already running\n";
        done2:;
        }

        // ---------------- remove ----------------
        else if (cmd == "remove") {
            string nodeId;
            ss >> nodeId;

            for (auto& rn : nodes) {
                if (rn.cfg.nodeId == nodeId) {
                    if (rn.alive) {
                        rn.node->stop();
                        rn.node.reset();
                    }
                    rn.alive = false;
                    cout << "[CLUSTER] Removed node " << nodeId << "\n";
                    goto done3;
                }
            }
            cout << "[ERROR] Node not found\n";
        done3:;
        }

        // ---------------- list ----------------
        else if (cmd == "list") {
            for (auto& rn : nodes) {
                cout << rn.cfg.nodeId
                     << " shard=" << rn.cfg.shardId
                     << " alive=" << (rn.alive ? "yes" : "no")
                     << "\n";
            }
        }

        // ---------------- peers ----------------
        else if (cmd == "peers") {
            string nodeId;
            ss >> nodeId;

            RunningNode* target = nullptr;
            for (auto& rn : nodes) {
                if (rn.cfg.nodeId == nodeId && rn.alive) {
                    target = &rn;
                    break;
                }
            }

            if (!target) {
                cout << "[ERROR] Node not alive\n";
                continue;
            }

            zmq::socket_t cli(cliCtx, ZMQ_REQ);
            cli.setsockopt(ZMQ_RCVTIMEO, 1000);

            string ep =
                "tcp://" + target->cfg.host + ":" +
                to_string(target->cfg.clientPort);

            cli.connect(ep);

            Message req = Message::get_nodes("cluster", Util::now_ms());
            cli.send(req.to_zmq(), zmq::send_flags::none);

            zmq::message_t reply;
            if (!cli.recv(reply, zmq::recv_flags::none)) {
                cout << "[ERROR] No response\n";
                continue;
            }

            Message resp = Message::from_zmq(reply);
            for (auto& n : resp.nodes) {
                cout << "  " << n.nodeId
                     << " shard=" << n.shardId
                     << " lastSeenTs=" << n.lastSeenTs
                     << "\n";
            }
        }

        // ---------------- quit ----------------
        else if (cmd == "quit") {
            break;
        }

        else {
            cout << "[ERROR] Unknown command\n";
        }
    }

    // ----------------------------
    // Shutdown
    // ----------------------------
    cout << "[CLUSTER] Shutting down...\n";
    for (auto& rn : nodes) {
        if (rn.alive) rn.node->stop();
    }

    return 0;
}