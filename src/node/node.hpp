#pragma once
#include <zmq.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include "shopping_list.hpp"
#include "../persistence/sqlite_db.hpp"

class Node{
    public:
        Node(const std::string &nodeID,
            int shardID,
            int numShards,
            int numReplicas,
        );

        ~Node();

        void start();
        void stop();

        /// current understanding of communication model:
        // client/proxy request is directed to a single shard (ex req/rep?)
        // replicas apply (via CRDT) and broadcast to each other all operations they receive from client (or a proxy)
        // communication between replicas is via channel/topic for their shared shard
        // operations received from other replicas are not further broadcasted, and are applied using crdt


        // helpers for local simulations
        void broadcastOperation(const std::string &op, const std::string &listUID);
        ShoppingList createList();
        ShoppingList addItem(const std::string &listUID,
                            const std::string &itemName,
                            int desiredQuantity,
                            int currentQuantity);
        ShoppingList updateItem(const std::string &listUID,
                                const std::string &itemUID,
                                const std::string &itemName,
                                int desiredQuantity,
                                int currentQuantity);
        ShoppingList removeItem(const std::string &listUID,
                                const std::string &itemUID);
        ShoppingList getList(const std::string &listUID);

    private:

        void mainLoop();
        void processIncoming(const std::string &msg, int shardID);
        void broadcast(const std::string &msg, int shardID);
        int getShardID(const std::string &listUID);

        std::string nodeID_;
        int shardID_;
        int numShards_;
        int numReplicas_;

        SqliteDb db;

        zmq::context_t ctx;
        zmq::socket_t proxy;
        zmq::socket_t shard;

        std::atomic<bool> running;
        std::thread mainThread;
};
