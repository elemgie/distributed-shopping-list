#ifndef SHOPPING_ITEM_HPP
#define SHOPPING_ITEM_HPP

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

using namespace std;

class ShoppingItem {
    private:
        string uid;
        string name;
        uint32_t desiredQuantity;
        uint32_t currentQuantity;
        uint32_t lastModificationTs;

    public:
        ShoppingItem(string uid, const string& name, uint32_t desiredQuantity,
            uint32_t currentQuantity);
        string getUid() const;
        string getName() const;
        uint32_t getDesiredQuantity() const;
        uint32_t getCurrentQuantity() const;
        uint32_t getLastModificationTs() const;
        void setCurrentQuantity(uint32_t quantity);
        void setDesiredQuantity(uint32_t quantity);
        void setName(const string& name);
        void setLastModificationTs(uint32_t ts);
        static nlohmann::json to_json(const ShoppingItem& it);
};

#endif