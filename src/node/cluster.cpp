#include "node.hpp"
#include "../message/message.hpp"
#include "../util.cpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <csignal>

using namespace std;
using namespace message;

static void sleep_ms(int ms) {
    this_thread::sleep_for(std::chrono::milliseconds(ms));
}

volatile std::sig_atomic_t stop_flag = 0;

void handle_sigint(int) {
    stop_flag = 1;
}

int main() {
    std::signal(SIGINT, handle_sigint);
    std::signal(SIGTERM, handle_sigint);

    const int numShards = 2;
    const int replicasPerShard = 3;
    const int N = numShards * replicasPerShard;
    const int baseClientPort = 5000;
    const int baseGossipPullPort = 7000;
    const int baseDiscoveryPullPort = 8000;

    // ----------------------------
    // Build configs
    // ----------------------------
    vector<NodeConfig> cfgs(N);
    for (int i = 0; i < N; i++) {
        cfgs[i].nodeId   = "N" + to_string(i);
        cfgs[i].host     = "127.0.0.1";
        cfgs[i].clientPort      = baseClientPort + i;
        cfgs[i].gossipPullPort  = baseGossipPullPort + i;
        cfgs[i].discoveryPullPort = baseDiscoveryPullPort + i;
        cfgs[i].gossipPushPort = 0; // Not used directly
        cfgs[i].discoveryPushPort = 0; // Not used directly
        cfgs[i].numShards = numShards;
        cfgs[i].shardId   = i / replicasPerShard;
        cfgs[i].gossipIntervalMs = 300;
        cfgs[i].discoveryIntervalMs = 1000;
        cfgs[i].discoveryTimeoutMs = 5000;
        cfgs[i].dbPath = "db/" + to_string(i) + "test.db";
    }

    vector<NodeInfo> allNodeInfos(N);
    for (int i = 0; i < N; i++) {
        allNodeInfos[i] = NodeInfo{
            cfgs[i].nodeId,
            cfgs[i].host,
            cfgs[i].shardId,
            cfgs[i].clientPort,
            cfgs[i].gossipPullPort,
            cfgs[i].discoveryPullPort,
        };
    }

    // Assign initialPeers for each node (all except self)
    for (int i = 0; i < N; i++) {
        cfgs[i].initialPeers.clear();
        for (int j = 0; j < N; j++) {
            if (i != j) cfgs[i].initialPeers.push_back(allNodeInfos[j]);
        }
    }

    vector<unique_ptr<Node>> nodes;
    for (int i = 0; i < N; i++) {
        nodes.emplace_back(new Node(cfgs[i]));
        nodes[i]->start();
    }

    cout << "[TEST] Booting nodes..." << endl;
    sleep_ms(600);

    cout << "[TEST] Nodes running. Press Ctrl+C to exit." << endl;
    while (!stop_flag) {
        sleep_ms(1000);
    }
    cout << "\n[TEST] Shutting down..." << endl;
    for (auto& n : nodes) n->stop();
    return 0;
}
