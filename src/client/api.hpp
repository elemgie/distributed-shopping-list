#include "shopping_list.hpp"

class API {
    public:
        ShoppingList createShoppingList(string name);
        ShoppingList addItem(const string& listUID, string itemName, int desiredQuantity, int currentQuantity);
        ShoppingList updateItem(const string& listUID, const string& itemUID, string itemName, int desiredQuantity, int currentQuantity);
        ShoppingList removeItem(const string& listUID, const string& itemUID);
        ShoppingList getShoppingList(const string& listUID);
        void deleteShoppingList(const string& listUID);
};