#include "api.hpp"

#include <random>
#include <unordered_map>
#include <ctime>

using namespace std;


// TODO: change to persistent storage
unordered_map<string, ShoppingList> shoppingLists;

static const char chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

string createUID(size_t length) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, length - 1);

    string id(length, 'x');
    do {
        for (auto &c : id) c = chars[dist(rng)];
    } while (shoppingLists.find(id) != shoppingLists.end());
    return id;
}

ShoppingList API::createShoppingList() {
    string uid = createUID(32);
    ShoppingList lst(uid);
    shoppingLists[uid] = lst;
    return lst;
}

ShoppingList API::addItem(const string& listUID, string itemName, int desiredQuantity, int currentQuantity) {
    ShoppingItem item(createUID(32), itemName, desiredQuantity, currentQuantity);
    item.setLastModificationTs(static_cast<uint32_t>(time(nullptr)));
    shoppingLists[listUID].add(item);
    return shoppingLists[listUID];
}

ShoppingList API::updateItem(const string& listUID, const string& itemUID, string itemName, int desiredQuantity, int currentQuantity) {
    ShoppingItem& item = shoppingLists[listUID].getItem(itemUID);
    item.setName(itemName);
    item.setDesiredQuantity(desiredQuantity);
    item.setCurrentQuantity(currentQuantity);
    item.setLastModificationTs(static_cast<uint32_t>(time(nullptr)));
    return shoppingLists[listUID];
}

ShoppingList API::removeItem(const string& listUID, const string& itemUID) {
    ShoppingItem& item = shoppingLists[listUID].getItem(itemUID);
    shoppingLists[listUID].remove(item);
    return shoppingLists[listUID];
}

void API::deleteShoppingList(const string& listUID) {
    shoppingLists.erase(listUID);
}