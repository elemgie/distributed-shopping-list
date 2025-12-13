#ifndef API_HPP
#define API_HPP

#include <zmq.hpp>
#include <string>
#include <mutex>
#include <random>
#include <ctime>
#include <stdexcept>
#include <zmq.hpp>
#include <random>

#include "../model/shopping_list.hpp"
#include "../persistence/sqlite_db.hpp"
#include "../message/message.hpp"
#include "util.cpp"


class API
{
public:
    API(SqliteDb *db);
    ~API();
    ShoppingList getShoppingList(const std::string &listUID);
    ShoppingList createShoppingList(const std::string &name);
    ShoppingList addItem(const std::string &listUID, std::string itemName, int desiredQuantity, int currentQuantity);
    ShoppingList updateItem(const std::string &listUID, const std::string &itemUID, std::string itemName, int desiredQuantity, int currentQuantity);
    ShoppingList removeItem(const std::string &listUID, const std::string &itemUID);
    void deleteShoppingList(const std::string &listUID);

private:
    SqliteDb *db;
    zmq::context_t ctx;
    zmq::socket_t clientSocket;
    std::string currentEndpoint;
    std::mutex sockMutex;
    std::string createUID(size_t length = 32);
    unordered_map<int, std::vector<std::string>> shardEndpoints;
    std::mt19937 randomEngine;
    int cloudTimeoutMs = 150;

    string getShardEndpoint(const string& listUID);
    string getShardEndpoint(const ShoppingList& list);
    void setNodeEndpoint(const std::string &nodeEndpoint);
    void resetSocket();
    message::Message sendCloudMessage(string receiverAddress, const message::Message& m);
};

#endif
