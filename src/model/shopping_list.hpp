#ifndef SHOPPING_LIST_HPP
#define SHOPPING_LIST_HPP

#include <map>
#include <vector>
#include <string>
#include <msgpack.hpp>

#include "shopping_item.hpp"
#include "../crdt/or_set.hpp"


using namespace std;

class ShoppingList {
    private:
        string uid;
        string name;
        ORSet<ShoppingItem> items;

    
    public:
        ShoppingList() = default;
        string getUid() const;
        ShoppingList(string uid, string name);
        void add(const ShoppingItem& item);
        void update(const ShoppingItem& item);
        void remove(const ShoppingItem& item);
        bool contains(const ShoppingItem& item) const;
        ShoppingItem& getItem(const string& uid);
        const ShoppingItem& getItem(const string& uid) const;
        vector<ShoppingItem*> getAllItems();
        vector<const ShoppingItem*> getAllItems() const;
        friend nlohmann::json to_json(const ShoppingList& lst);
        void merge(const ShoppingList &other);

        MSGPACK_DEFINE(uid, name, items);
    };

namespace std {
    template<>
    struct hash<ShoppingList> {
        size_t operator()(const ShoppingList &lst) const noexcept {
            return hash<string>()(lst.getUid());
        }
    };
}

#endif