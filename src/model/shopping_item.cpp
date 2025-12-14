#include "shopping_item.hpp"
#include <ctime>
#include <iostream>

using json = nlohmann::json;
using namespace std;

ShoppingItem::ShoppingItem(string origin, string uid, const string& name, uint32_t desiredQuantity,
    uint32_t currentQuantity): uid(uid), name(name) {
            this->desiredQuantity.increment(origin, desiredQuantity);
            this->currentQuantity.increment(origin, currentQuantity);
        }

string ShoppingItem::getUid() const {
    return uid;
}

string ShoppingItem::getName() const {
    return name;
}

uint32_t ShoppingItem::getDesiredQuantity() const {
    return desiredQuantity.value();
}

uint32_t ShoppingItem::getCurrentQuantity() const {
    return currentQuantity.value();
}

void ShoppingItem::setCurrentQuantity(string origin, uint32_t quantity) {
    int32_t diff = static_cast<int32_t>(quantity) - static_cast<int32_t>(currentQuantity.value());
    if (diff > 0) {
        currentQuantity.increment(origin, diff);
    } else if (diff < 0) {
        currentQuantity.decrement(origin, -diff);
    }
}

void ShoppingItem::setDesiredQuantity(string origin, uint32_t quantity) {
        int32_t diff = static_cast<int32_t>(quantity) - static_cast<int32_t>(desiredQuantity.value());
    if (diff > 0) {
        desiredQuantity.increment(origin, diff);
    } else if (diff < 0) {
        desiredQuantity.decrement(origin, -diff);
    }
}

void ShoppingItem::setName(const string& name) {
    this -> name = name;
}

ShoppingItem& ShoppingItem::merge(const ShoppingItem &other) {
    if (this->uid != other.uid) {
        throw invalid_argument("Cannot merge ShoppingItems with different UIDs");
    }
    this->name = other.name;
    this->desiredQuantity.merge(other.desiredQuantity);
    this->currentQuantity.merge(other.currentQuantity);
    return *this;
}

json ShoppingItem::to_json(const ShoppingItem& it) {
    return json{{"uid", it.getUid()},
                {"name", it.getName()},
                {"desiredQuantity", it.getDesiredQuantity()},
                {"currentQuantity", it.getCurrentQuantity()}};
}