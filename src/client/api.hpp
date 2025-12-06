#ifndef API_HPP
#define API_HPP

#include "../model/shopping_list.hpp"
#include "../persistence/sqlite_db.hpp"
#include <string>

class API
{
public:
    API(SqliteDb *db);
    ShoppingList createShoppingList(const std::string &name);
    ShoppingList addItem(const std::string &listUID, std::string itemName, int desiredQuantity, int currentQuantity);
    ShoppingList updateItem(const std::string &listUID, const std::string &itemUID, std::string itemName, int desiredQuantity, int currentQuantity);
    ShoppingList removeItem(const std::string &listUID, const std::string &itemUID);
    void deleteShoppingList(const std::string &listUID);

private:
    SqliteDb *db;
    std::string createUID(size_t length = 32);
};

#endif
