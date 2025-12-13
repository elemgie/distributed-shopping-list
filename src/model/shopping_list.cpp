#include "shopping_list.hpp"
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

ShoppingList::ShoppingList(string uid, string name): uid(uid), name(name) {}

string ShoppingList::getUid() const {
    return uid;
}

void ShoppingList::add(const ShoppingItem& item) {
    if (this -> contains(item)) {
        throw invalid_argument("Item with the same UID already exists in the shopping list");
    }
    items.emplace(item.getUid(), item);
}

bool ShoppingList::remove(const ShoppingItem& item) {
    return items.erase(item.getUid()) > 0;
}

bool ShoppingList::contains(const ShoppingItem& item) const {
    return items.find(item.getUid()) != items.end();
}

ShoppingItem& ShoppingList::getItem(string uid) {
    auto it = items.find(uid);
    if (it == items.end()) {
        throw std::invalid_argument("Item with the given UID does not exist in the shopping list");
    }
    return it->second;
}

vector<ShoppingItem*> ShoppingList::getAllItems() {
    vector<ShoppingItem*> allItems;
    allItems.reserve(items.size());
    for (auto& pair : items) {
        allItems.push_back(&pair.second);
    }
    return allItems;
}

json to_json(ShoppingList& list) {
    json items = json::array();
    for (auto *it : list.getAllItems()) {
        items.push_back(ShoppingItem::to_json(*it));
    }
    return json{{"items", items}, {"uid", list.uid}, {"name", list.name}};
}