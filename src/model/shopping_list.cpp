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
    items.add(item);
}

void ShoppingList::update(const ShoppingItem& item) {
    if (!this -> contains(item)) {
        throw invalid_argument("Item not found in the shopping list");
    }
    items.add(item);
}

void ShoppingList::remove(const ShoppingItem& item) {
    items.remove(item);
}

bool ShoppingList::contains(const ShoppingItem& item) const {
    return items.contains(item);
}

ShoppingItem& ShoppingList::getItem(const string& uid) {
    for (auto* it : items.values()) {
        if (it->getUid() == uid) { // TODO: rethink and rework so we don't have linear search!
            return *it;
        }
    }
    throw invalid_argument("Item not found in the shopping list");
}

const ShoppingItem& ShoppingList::getItem(const string& uid) const {
    for (auto* it : items.values()) {
        if (it->getUid() == uid) {
            return *it;
        }
    }
    throw invalid_argument("Item not found in the shopping list");
}

vector<ShoppingItem*> ShoppingList::getAllItems() {
    return items.values();
}

vector<const ShoppingItem*> ShoppingList::getAllItems() const {
    return items.values();
}

json to_json(const ShoppingList& list) {
    json items = json::array();
    for (auto* it : list.getAllItems()) {
        items.push_back(ShoppingItem::to_json(*it));
    }
    return json{{"items", items}, {"uid", list.uid}, {"name", list.name}};
}

void ShoppingList::merge(const ShoppingList &other) {
    this->items.merge(other.items);
}