#ifndef SHOPPING_LIST_HPP
#define SHOPPING_LIST_HPP

#include <map>
#include <vector>
#include <string>
#include <msgpack.hpp>

#include "shopping_item.hpp"

using namespace std;

class ShoppingList {
    private:
        string uid;
        string name;
        map<string, ShoppingItem> items;
    
    public:
        ShoppingList() = default;
        string getUid() const;
        ShoppingList(string uid, string name);
        void add(const ShoppingItem& item);
        bool remove(const ShoppingItem& item);
        bool contains(const ShoppingItem& item) const;
        ShoppingItem& getItem(string uid);
        vector<ShoppingItem*> getAllItems();
        friend nlohmann::json to_json(ShoppingList& list);
        MSGPACK_DEFINE(uid, name, items);
};

#endif