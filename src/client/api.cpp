#include "api.hpp"
#include <iostream>

using namespace std;
using namespace message;

API::API(SqliteDb* db)
: db(db), ctx(1), clientSocket(ctx, zmq::socket_type::req)
{
    clientSocket.set(zmq::sockopt::sndtimeo, cloudTimeoutMs);
    clientSocket.set(zmq::sockopt::rcvtimeo, cloudTimeoutMs);
    shardEndpoints = unordered_map<int, vector<string>>{
        {0, vector<string>{"tcp://127.0.0.1:5000", "tcp://127.0.0.1:5001", "tcp://127.0.0.1:5002"}},
        {1, vector<string>{"tcp://127.0.0.1:5003", "tcp://127.0.0.1:5004", "tcp://127.0.0.1:5005"}}
    };
    randomEngine = mt19937{random_device{}()};
}

API::~API()
{
    int linger = 0;
    clientSocket.set(zmq::sockopt::linger, 0);
    clientSocket.close();
}

void API::setNodeEndpoint(const string &nodeEndpoint)
{
    if (currentEndpoint == nodeEndpoint) return;
    if (!currentEndpoint.empty()) {
        clientSocket.disconnect(currentEndpoint);
    }
    clientSocket.connect(nodeEndpoint);
    currentEndpoint = nodeEndpoint;
}

string API::getShardEndpoint(const ShoppingList& lst) {
    return getShardEndpoint(lst.getUid());
}

string API::getShardEndpoint(const string& listUID) {
    int shard = Util::getHash(listUID) % shardEndpoints.size();
    const vector<string>& endpoints = shardEndpoints[shard];
    uniform_int_distribution<int> dist(0, endpoints.size() - 1);
    return endpoints[dist(randomEngine)];
}

string API::createUID(size_t length) {
    uniform_int_distribution<int> dist(0, 61);
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string id(length, 'x');
    do
    {
        for (auto &c : id)
            c = chars[dist(randomEngine)];
    } while (db->read(id).has_value());
    return id;
}

void API::resetSocket() {
    clientSocket.set(zmq::sockopt::linger, 0);
    clientSocket.close();
    clientSocket = zmq::socket_t(ctx, zmq::socket_type::req);
    clientSocket.set(zmq::sockopt::sndtimeo, cloudTimeoutMs);
    clientSocket.set(zmq::sockopt::rcvtimeo, cloudTimeoutMs);
    if (!currentEndpoint.empty()) clientSocket.connect(currentEndpoint);
}

Message API::sendCloudMessage(string receiverAddress, const Message& m) {
    lock_guard<mutex> g(sockMutex);
    setNodeEndpoint(receiverAddress);

    clientSocket.set(zmq::sockopt::sndtimeo, cloudTimeoutMs);
    clientSocket.set(zmq::sockopt::rcvtimeo, cloudTimeoutMs);

    try {
        // cout << "Sending message to " << receiverAddress << endl;
        clientSocket.send(m.to_zmq(), zmq::send_flags::none);

        zmq::message_t reply;
        zmq::recv_result_t result = clientSocket.recv(reply);

        if (!result) { // socket timeout
            resetSocket();
            throw runtime_error("CLOUD_TIMEOUT");
        }
        // cout << "Got meaningful reply from " << receiverAddress << endl;
        // cout << reply << endl;
        return Message::from_zmq(reply);
    } catch (const zmq::error_t& e) {
        cerr << "ZMQ error: " << e.what() <<  endl;
        resetSocket();
        throw runtime_error(string("CLOUD_ZMQ_ERROR: ") + e.what());
    }
    catch (const std::exception& e) {
        cerr << "General error: " << e.what() << endl;
        throw e;
    }
}

ShoppingList API::createShoppingList(const string &name) {
    string uid = createUID();
    ShoppingList lst(uid, name);
    db->write(lst);

    Message m = Message::ensure_list("", Util::now_ms(), lst);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(lst), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            lst = reply.lists[0];
            // TODO: local crdt merge here
        }
    } catch (const exception& e) {}

    return lst;
}

ShoppingList API::addItem(const string &listUID, string itemName, int desiredQuantity, int currentQuantity) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList lst = move(optList.value());
    ShoppingItem item(createUID(), itemName, desiredQuantity, currentQuantity);
    lst.add(item);
    db->write(lst);

    Message m = Message::ensure_list("", Util::now_ms(), lst);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(lst), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            lst = reply.lists[0];
            // TODO: local crdt merge here
        }
    } catch (const exception& e) {}

    return lst;
}

ShoppingList API::updateItem(const string &listUID, const string &itemUID, string itemName, int desiredQuantity, int currentQuantity) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList lst = move(optList.value());

    ShoppingItem &item = lst.getItem(itemUID);
    item.setName(itemName);
    item.setDesiredQuantity(desiredQuantity);
    item.setCurrentQuantity(currentQuantity);
    db->write(lst);

    Message m = Message::ensure_list("", Util::now_ms(), lst);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(lst), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            lst = reply.lists[0];
            // TODO: local crdt merge here
        }
    } catch (const exception& e) {}
    return lst;
}

ShoppingList API::removeItem(const string &listUID, const string &itemUID) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList lst = move(optList.value());

    ShoppingItem &item = lst.getItem(itemUID);
    lst.remove(item);
    db->write(lst);

    Message m = Message::ensure_list("", Util::now_ms(), lst);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(lst), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            lst = reply.lists[0];
            // TODO: local crdt merge here
        }
    } catch (const exception& e) {}

    return lst;
}

ShoppingList API::getShoppingList(const string& listUID) {
    optional<ShoppingList> optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");

    ShoppingList lst = move(optList.value());
    Message m = Message::get_list("", Util::now_ms(), listUID);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(lst), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            lst = reply.lists[0];
            // TODO: local crdt merge here
        }
    } catch (const exception& e) {}

    return lst;
}

void API::deleteShoppingList(const string &listUID) {
    db->delete_list(listUID);

    Message m = Message::delete_list("", Util::now_ms(), listUID);
    try {
        Message reply = sendCloudMessage(getShardEndpoint(listUID), m);
        if (reply.op == OpType::LIST_RESPONSE) {
            cerr << "WEIRD" << endl;
        }
        else if (reply.op == OpType::NO_LIST_RESPONSE) {
            cout << "Tudo bem" << endl;
        }
    } catch (const exception& e) {}
}
