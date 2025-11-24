#include <zmq.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

#include "model/shopping_list.hpp"

using namespace std;

unordered_map<string, ShoppingList> lists;

static const char chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

string gen_id(const char availableCharacters[], size_t length) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, length - 1);

    std::string id(8, 'x');
    for (auto &c : id) c = availableCharacters[dist(rng)];
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
    cout << "Connected to server.\n";
    cout << "Commands:\n"
         << "  CREATE\n"
         << "  ADD <list_id> <item text>\n"
         << "  GET <list_id>\n"
         << "Type EXIT to quit.\n\n";

    while (true) {
        cout << "> ";
        string req;
        getline(cin, req);

        if (req == "EXIT" || cin.eof()) break;
        if (req.empty()) continue;

        auto tokens = split(req);
        string rep;

        if (tokens.empty()) {
            rep = "ERR empty_request";
        }
        else if (tokens[0] == "CREATE") {
            string id = gen_id(chars, 32);
            lists[id] = ShoppingList(id);
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
                    if (pos ==  ::string::npos || pos + 1 >= req.size()) {
                        rep = "ERR missing_item_text";
                    } else {
                        string item = req.substr(pos + 1);
                        it->second.add(ShoppingItem(gen_id(chars, 32), item, 1, 0));
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
                    size_t count = 1;
                    for (ShoppingItem* i: it->second.getAllItems()) {
                        oss << count++ << " " << i -> getName() << ": " <<
                            i -> getCurrentQuantity() << "/" << i -> getDesiredQuantity() << "\n";
                    }
                    rep = oss.str();
                }
            }
        }
        else {
            rep = "ERR unknown_command";
        }

        cout << rep;
    }
    return 0;
}
