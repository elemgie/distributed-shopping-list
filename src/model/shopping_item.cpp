#include "shopping_item.hpp"
#include <ctime>

using json = nlohmann::json;
using namespace std;

ShoppingItem::ShoppingItem(string uid, const string& name, uint32_t desiredQuantity,
    uint32_t currentQuantity) : uid(uid), name(name), desiredQuantity(desiredQuantity),
      currentQuantity(currentQuantity) {}

string ShoppingItem::getUid() const {
    return uid;
}

string ShoppingItem::getName() const {
    return name;
}

uint32_t ShoppingItem::getDesiredQuantity() const {
    return desiredQuantity;
}

uint32_t ShoppingItem::getCurrentQuantity() const {
    return currentQuantity;
}

void ShoppingItem::setCurrentQuantity(uint32_t quantity) {
    currentQuantity = quantity;
}

void ShoppingItem::setDesiredQuantity(uint32_t quantity) {
    desiredQuantity = quantity;
}

void ShoppingItem::setName(const string& name) {
    this -> name = name;
}

json ShoppingItem::to_json(const ShoppingItem& it) {
    return json{{"uid", it.getUid()},
                {"name", it.getName()},
                {"desiredQuantity", it.getDesiredQuantity()},
                {"currentQuantity", it.getCurrentQuantity()}};
}