#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <zmq.hpp>
#include "../persistence/sqlite_db.hpp"
#include "../message/message.hpp"
#include "../model/shopping_list.hpp"
#include "../model/shopping_item.hpp"

struct NodeConfig {
    std::string nodeId;
    std::string host;
    int clientPort;
    int gossipPushPort;
    int gossipPullPort;
    int discoveryPushPort;
    int discoveryPullPort;
    std::vector<message::NodeInfo> initialPeers; // replaces replicaPullEndpoints and discoveryPeers
    std::string dbPath;
    int shardId;
    int numShards;
    int gossipIntervalMs;
    int discoveryIntervalMs;
    int discoveryTimeoutMs;
};

class Node {
public:
    explicit Node(const NodeConfig& cfg);
    ~Node();

    void start();
    void stop();
    void run_loop();

private:
    void handle_client_frame();
    void handle_gossip_frame();
    void handle_discovery_frame();

    void apply_message(const message::Message& m);
    void eager_fanout();
    void perform_shard_gossip();
    void perform_discovery_gossip();
    void evict_dead_nodes();
    void update_known_nodes(const std::vector<message::NodeInfo>& nodes);

    int shard_for_list(const std::string& listId) const;
    uint64_t next_gossip_ts(uint64_t interval) const;

    void handle_peer(const message::NodeInfo& n, uint64_t now = 0);

private:
    NodeConfig cfg;
    zmq::context_t ctx;
    zmq::socket_t repSock;
    zmq::socket_t gossipPushSock;
    zmq::socket_t gossipPullSock;
    zmq::socket_t discoveryPushSock;
    zmq::socket_t discoveryPullSock;

    std::unordered_map<std::string, message::NodeInfo> knownNodes;
    std::unordered_set<std::string> connectedDiscovery;
    std::unordered_set<std::string> connectedShard;

    SqliteDb db;
    std::thread loopThread;
    std::atomic<bool> running;
};

#endif
