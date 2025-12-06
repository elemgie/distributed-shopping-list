#ifndef DB_HPP
#define DB_HPP

#include "../model/shopping_item.hpp"
#include "../model/shopping_list.hpp"

#include <optional>
#include <vector>
#include <string>

class IDb {
public:
    virtual ~IDb() = default;

    virtual bool init_db(const std::string& dbPath) = 0;

    virtual bool write(const ShoppingList& list) = 0;
    
    virtual std::optional<ShoppingList> read(const std::string& listId) = 0;

    virtual bool delete_list(const std::string& listId) = 0;

    virtual bool write_many(const std::vector<ShoppingList>& lists) = 0;

    virtual std::vector<std::optional<ShoppingList>> read_many(const std::vector<std::string>& listIds) = 0;

    virtual bool delete_many(const std::vector<std::string>& listIds) = 0;

    virtual std::vector<ShoppingList> read_all() = 0;

    virtual std::vector<std::string> get_all_list_ids() = 0;
};

#endif
