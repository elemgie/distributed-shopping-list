#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <zmq.hpp>
#include "../persistence/sqlite_db.hpp"
#include "../message/message.hpp"
#include "../model/shopping_list.hpp"
#include "../model/shopping_item.hpp"

struct NodeConfig {
    std::string nodeId;
    std::string host;           // e.g. "127.0.0.1"
    int clientPort;             // REP port for clients
    int pubPort;                // this node's PUB port (its own endpoint)
    int gossipPushPort;         // this node PUSHes to peer pull ports
    int gossipPullPort;         // this node listens on this PULL port
    std::vector<std::string> peerPubEndpoints; // other replicas' pub endpoints (tcp://host:port)
    std::vector<std::string> peerGossipPullEndpoints; // "tcp://host:port" entries
    std::string dbPath;
    int shardId;                // shard this node is responsible for
    int numShards;
    int replicationFactor;
    int gossipIntervalMs;
};

class Node {
public:
    explicit Node(const NodeConfig& cfg);
    ~Node();

    void start();
    void stop();
    void run_loop();            // internal loop (public for simulation convenience)
    void addPeerPubEndpoint(const std::string& ep);

private:
    void handle_client_frame();
    void handle_sub_frames();
    void handle_gossip_frame();

    void apply_message(const message::Message& m);
    void broadcast_message(const message::Message& m);
    void perform_gossip();

    int shard_for_list(const std::string& listId) const;
    uint64_t next_gossip_ts() const;

private:
    NodeConfig cfg;
    zmq::context_t ctx;
    zmq::socket_t repSock;
    zmq::socket_t pubSock;
    zmq::socket_t subSock;
    zmq::socket_t gossipPushSock;
    zmq::socket_t gossipPullSock;


    SqliteDb db;
    std::thread loopThread;
    std::atomic<bool> running;
};

#endif // NODE_HPP
