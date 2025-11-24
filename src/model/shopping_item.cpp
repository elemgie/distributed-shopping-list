#include "shopping_item.hpp"
#include <ctime>

using namespace std;

ShoppingItem::ShoppingItem(string uid, const string& name, uint32_t desiredQuantity,
    uint32_t currentQuantity)
    : uid(uid), name(name), desiredQuantity(desiredQuantity),
      currentQuantity(currentQuantity) {
    lastModificationTs = static_cast<uint32_t>(time(nullptr));
}

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

uint32_t ShoppingItem::getLastModificationTs() const {
    return lastModificationTs;
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

void ShoppingItem::setLastModificationTs(uint32_t ts) {
    lastModificationTs = ts;
}