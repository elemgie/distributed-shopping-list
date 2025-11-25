//hash function, network of comunication, last right policy, api, the servers have to connect and get and update a list

#include <zmq.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <functional>
#include <chrono>
#include <thread>

#include "model/shopping_list.hpp"
#include "model/shopping_item.hpp"
#include "client/api.hpp"

using namespace std;

//Hash Func

size_t hash_list (const string& uid){
    hash <string> h;
    return h(uid);
}

//Local storage for this server shard

unordered_map<string , ShoppingList> shard;

//split utility

vector <string> split (const string& s){
    istringstream iss(s);
    vector <string> v;
    string p;
    while (iss >> p) v.push_back(p);
    return v;

}

//Last-writer policy

void lwwApply(const string& listID, const string& itemUID, const ShoppingItem& incoming) {
    ShoppingList& lst = shard[listID];

    try {
        ShoppingItem& existing = lst.getItem(itemUID);

        if (incoming.getLastModificationTs() > existing.getLastModificationTs()) {
            existing.setName(incoming.getName());
            existing.setDesiredQuantity(incoming.getDesiredQuantity());
            existing.setCurrentQuantity(incoming.getCurrentQuantity());
            existing.setLastModificationTs(incoming.getLastModificationTs());
        }
    }
    catch (...) {
        lst.add(incoming);
    }
}


int main(int argc, char* argv[]) {

    if (argc < 3) {
        cout << "Usage: ./server <serverID> <port>\n";
        return 1;
    }

    string serverID = argv[1];
    int port = stoi(argv[2]);

    zmq::context_t ctx(1);


    // REQUEST / RESPONSE (client → server)

    zmq::socket_t rep(ctx, zmq::socket_type::rep);
    rep.bind("tcp://*:" + to_string(port));


    // SUBSCRIBE TO REPLICATION CHANNEL

    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect("tcp://localhost:9000");
    sub.set(zmq::sockopt::subscribe, "");


    // UPDATES TO OTHER SERVERS

    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://*:" + to_string(port + 1000));

    cout << "Server " << serverID << " running on port " << port << "\n";

    zmq::pollitem_t items[] = {
        { static_cast<void*>(rep), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(sub), 0, ZMQ_POLLIN, 0 }
    };

    while (true) {

        zmq::poll(items, 2, chrono::milliseconds(50));


        // GET / UPDATE

        if (items[0].revents & ZMQ_POLLIN) {

            zmq::message_t req;
            rep.recv(req);

            string msg = req.to_string();
            vector<string> t = split(msg);

            if (t.empty()) {
                rep.send(zmq::buffer("ERR empty"), zmq::send_flags::none);
                continue;
            }

            // - GET 
            if (t[0] == "GET") {

                if (t.size() != 2) {
                    rep.send(zmq::buffer("ERR usage GET <listID>"), zmq::send_flags::none);
                    continue;
                }

                string listID = t[1];

                if (!shard.count(listID)) {
                    rep.send(zmq::buffer("ERR not_found"), zmq::send_flags::none);
                    continue;
                }

                ostringstream out;
                out << "OK\n";

                int n = 1;
                for (auto* item : shard[listID].getAllItems()) {
                    out << n++ << " "
                        << item->getUid() << " "
                        << item->getName() << " "
                        << item->getCurrentQuantity() << "/"
                        << item->getDesiredQuantity() << " ts="
                        << item->getLastModificationTs() << "\n";
                }

                rep.send(zmq::buffer(out.str()), zmq::send_flags::none);
            }

            // - UPDATE 
            else if (t[0] == "UPDATE") {

                if (t.size() < 7) {
                    rep.send(zmq::buffer("ERR usage UPDATE <listID> <itemUID> <name> <desired> <current> <timestamp>"),
                             zmq::send_flags::none);
                    continue;
                }

                string listID = t[1];
                string itemUID = t[2];

                size_t pos = msg.find(itemUID) + itemUID.size();
                pos = msg.find(" ", pos) + 1;

                vector<string> parts = split(msg.substr(pos));

                string name = parts[0];
                int desired = stoi(parts[1]);
                int current = stoi(parts[2]);
                uint32_t ts = stoul(parts[3]);

                ShoppingItem item(itemUID, name, desired, current);
                item.setLastModificationTs(ts);

                shard[listID];
                lwwApply(listID, itemUID, item);

                pub.send(zmq::buffer(msg), zmq::send_flags::none);

                rep.send(zmq::buffer("OK"), zmq::send_flags::none);
            }

            // UNKNOWN
            else {
                rep.send(zmq::buffer("ERR unknown"), zmq::send_flags::none);
            }
        }

        // 
        // REPLICATION (updates from other servers)
        //
        if (items[1].revents & ZMQ_POLLIN) {

            zmq::message_t r;
            sub.recv(r);

            string msg = r.to_string();
            vector<string> t = split(msg);

            if (t[0] == "UPDATE" && t.size() >= 7) {

                string listID = t[1];
                string itemUID = t[2];

                size_t pos = msg.find(itemUID) + itemUID.size();
                pos = msg.find(" ", pos) + 1;

                vector<string> parts = split(msg.substr(pos));

                string name = parts[0];
                int desired = stoi(parts[1]);
                int current = stoi(parts[2]);
                uint32_t ts = stoul(parts[3]);

                ShoppingItem item(itemUID, name, desired, current);
                item.setLastModificationTs(ts);

                shard[listID];
                lwwApply(listID, itemUID, item);
            }
        }
    }

    return 0;
}