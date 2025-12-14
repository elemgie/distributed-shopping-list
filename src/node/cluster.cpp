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

    // ----------------------------
    // Build configs
    // ----------------------------
    vector<NodeConfig> cfgs(N);

    for (int i = 0; i < N; i++) {
        cfgs[i].nodeId   = "N" + to_string(i);
        cfgs[i].host     = "127.0.0.1";
        cfgs[i].clientPort      = 5000 + i;
        cfgs[i].pubPort         = 6000 + i;   // unused, but must bind
        cfgs[i].gossipPullPort  = 7000 + i;

        cfgs[i].numShards = numShards;
        cfgs[i].shardId   = (i < replicasPerShard) ? 0 : 1;

        cfgs[i].gossipIntervalMs = 300; // fast for test
        cfgs[i].dbPath = "db/" + to_string(i) + "test.db";
        cfgs[i].shardId   = i / replicasPerShard;
        cfgs[i].replicationFactor = replicasPerShard;
    }

    // ----------------------------
    // Fully interconnect each shard (gossip)
    // ----------------------------
    auto add_edge = [&](int from, int to) {
        string pull = "tcp://" + cfgs[to].host + ":" + to_string(cfgs[to].gossipPullPort);
        cfgs[from].peerGossipPullEndpoints.push_back(pull);
    };

    for (int shard = 0; shard < numShards; ++shard) {
        int start = shard * replicasPerShard;
        int end = start + replicasPerShard;
        for (int i = start; i < end; ++i) {
            for (int j = start; j < end; ++j) {
                if (i != j) add_edge(i, j);
            }
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
