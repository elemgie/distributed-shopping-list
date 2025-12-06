#include "api.hpp"
#include <random>
#include <ctime>
#include <stdexcept>

using namespace std;

API::API(SqliteDb *db) : db(db) {}

string API::createUID(size_t length) {
    static thread_local mt19937_64 rng{random_device{}()};
    uniform_int_distribution<int> dist(0, 61);
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string id(length, 'x');
    do
    {
        for (auto &c : id)
            c = chars[dist(rng)];
    } while (db->read(id).has_value());
    return id;
}

ShoppingList API::createShoppingList(const string &name) {
    string uid = createUID();
    ShoppingList list(uid, name);
    db->write(list);
    return list;
}

ShoppingList API::addItem(const string &listUID, string itemName, int desiredQuantity, int currentQuantity) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList list = move(optList.value());
    ShoppingItem item(createUID(), itemName, desiredQuantity, currentQuantity);
    item.setLastModificationTs(static_cast<uint32_t>(time(nullptr)));
    list.add(item);
    db->write(list);
    return list;
}

ShoppingList API::updateItem(const string &listUID, const string &itemUID, string itemName, int desiredQuantity, int currentQuantity) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList list = move(optList.value());
    ShoppingItem &item = list.getItem(itemUID);
    item.setName(itemName);
    item.setDesiredQuantity(desiredQuantity);
    item.setCurrentQuantity(currentQuantity);
    item.setLastModificationTs(static_cast<uint32_t>(time(nullptr)));
    db->write(list);
    return list;
}

ShoppingList API::removeItem(const string &listUID, const string &itemUID) {
    auto optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    ShoppingList list = move(optList.value());
    ShoppingItem &item = list.getItem(itemUID);
    list.remove(item);
    db->write(list);
    return list;
}

ShoppingList API::getShoppingList(const string& listUID) {
    optional<ShoppingList> optList = db->read(listUID);
    if (!optList.has_value())
        throw runtime_error("List not found");
    return move(optList.value());
}

void API::deleteShoppingList(const string &listUID){
    db->delete_list(listUID);
}
