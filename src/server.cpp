#include <zmq.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <random>

using namespace std;

struct ShoppingList {
    vector<string> items;
};

unordered_map<string, ShoppingList> lists;

string gen_id() {
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, sizeof(chars)-2);

    std::string id(8, 'x');
    for (auto &c : id) c = chars[dist(rng)];
    return id;
}

vector<string> split(const string& s) {
    istringstream iss(s);
    vector<std::string> parts;
    string p;
    while (iss >> p) parts.push_back(p);
    return parts;
}

int main() {
    zmq::context_t ctx{1};
    zmq::socket_t sock{ctx, zmq::socket_type::rep};
    sock.bind("tcp://*:5555");

    cout << "Server on tcp://*:5555\n";

    while (true) {
        zmq::message_t req_msg;
        if (!sock.recv(req_msg, zmq::recv_flags::none)) continue;

        string req(static_cast<char*>(req_msg.data()), req_msg.size());
        auto tokens = split(req);
        string rep;

        if (tokens.empty()) {
            rep = "ERR empty_request";
        }
        else if (tokens[0] == "CREATE") {
            std::string id = gen_id();
            lists[id] = ShoppingList{};
            rep = "OK " + id;
        }
        else if (tokens[0] == "ADD") {
            if (tokens.size() < 3) {
                rep = "ERR usage: ADD <list_id> <item_text>";
            } else {
                std::string list_id = tokens[1];
                auto it = lists.find(list_id);
                if (it == lists.end()) {
                    rep = "ERR list_not_found";
                } else {
                    // rebuild item text from rest of line
                    size_t pos = req.find(list_id);
                    pos = req.find(' ', pos); // space after list_id
                    if (pos == std::string::npos || pos + 1 >= req.size()) {
                        rep = "ERR missing_item_text";
                    } else {
                        string item = req.substr(pos + 1);
                        it->second.items.push_back(item);
                        rep = "OK";
                    }
                }
            }
        }
        else if (tokens[0] == "GET") {
            if (tokens.size() != 2) {
                rep = "ERR usage: GET <list_id>";
            } else {
                string list_id = tokens[1];
                auto it = lists.find(list_id);
                if (it == lists.end()) {
                    rep = "ERR list_not_found";
                } else {
                    ostringstream oss;
                    oss << "OK\n";
                    for (size_t i = 0; i < it->second.items.size(); ++i) {
                        oss << i << " " << it->second.items[i] << "\n";
                    }
                    rep = oss.str();
                }
            }
        }
        else {
            rep = "ERR unknown_command";
        }

        zmq::message_t rep_msg(rep.size());
        memcpy(rep_msg.data(), rep.data(), rep.size());
        sock.send(rep_msg, zmq::send_flags::none);
    }
}
