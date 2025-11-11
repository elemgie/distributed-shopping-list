#ifndef SHOPPING_ITEM_HPP
#define SHOPPING_ITEM_HPP

#include <cstdint>
#include <string>

using namespace std;

class ShoppingItem {
    private:
        uint32_t uid;
        string name;
        uint32_t desiredQuantity;
        uint32_t currentQuantity;
        uint32_t lastModificationTs;

    public:
        ShoppingItem(uint32_t uid, const string& name, uint32_t desiredQuantity,
            uint32_t currentQuantity);
        uint32_t getUid() const;
        string getName() const;
        uint32_t getDesiredQuantity() const;
        uint32_t getCurrentQuantity() const;
        uint32_t getLastModificationTs() const;
        void setCurrentQuantity(uint32_t quantity);
        void setDesiredQuantity(uint32_t quantity);
        void setName(const string& name);
        void setLastModificationTs(uint32_t ts);
};

#endif