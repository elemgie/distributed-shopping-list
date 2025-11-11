#ifndef SHOPPING_LIST_HPP
#define SHOPPING_LIST_HPP

#include <map>
#include <vector>
#include <cstdint>

#include "shopping_item.hpp"

using namespace std;

class ShoppingList {
    private:
        uint32_t uid;
        map<uint32_t, ShoppingItem> items;
    
    public:
        ShoppingList();
        void add(const ShoppingItem& item);
        bool remove(const ShoppingItem& item);
        bool contains(const ShoppingItem& item) const;
        ShoppingItem& getItem(uint32_t uid);
        vector<ShoppingItem*> getAllItems();
};

#endif