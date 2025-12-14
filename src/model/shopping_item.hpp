#ifndef SHOPPING_ITEM_HPP
#define SHOPPING_ITEM_HPP

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <msgpack.hpp>
#include <functional>

#include "crdt/pn_counter.hpp"

using namespace std;

class ShoppingItem {
    private:
        string uid;
        string name;
        PNCounter desiredQuantity;
        PNCounter currentQuantity;

    public:
        ShoppingItem() = default;
        ShoppingItem(string origin, string uid, const string& name, uint32_t desiredQuantity,
            uint32_t currentQuantity);
        string getUid() const;
        string getName() const;
        uint32_t getDesiredQuantity() const;
        uint32_t getCurrentQuantity() const;
        void setCurrentQuantity(string origin, uint32_t quantity);
        void setDesiredQuantity(string origin, uint32_t quantity);
        void setName(const string& name);

        ShoppingItem& merge(const ShoppingItem &other);

        MSGPACK_DEFINE(uid, name, desiredQuantity, currentQuantity);
        static nlohmann::json to_json(const ShoppingItem& it);

        inline bool operator==(const ShoppingItem &x) const noexcept{
            return this -> uid == x.uid;
        }
};

namespace std {
    template<>
    struct hash<ShoppingItem> {
        size_t operator()(const ShoppingItem &item) const noexcept {
            return hash<string>()(item.getUid());
        }
    };
}

#endif