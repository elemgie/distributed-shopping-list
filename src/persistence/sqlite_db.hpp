#ifndef SQLITE_DB_HPP
#define SQLITE_DB_HPP

#include "db.hpp"
#include "../model/shopping_item.hpp"
#include "../model/shopping_list.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <optional>

class SqliteDb : public IDb {
public:
    ~SqliteDb() override;

    bool init_db(const std::string& dbPath) override;

    bool write(const ShoppingList& list) override;

    std::optional<ShoppingList> read(const std::string& listId) override;
    
    bool delete_list(const std::string& listId) override;

    bool write_many(const std::vector<ShoppingList>& lists) override;

    std::vector<std::optional<ShoppingList>> read_many(const std::vector<std::string>& listIds) override;
    
    bool delete_many(const std::vector<std::string>& listIds) override;

    std::vector<ShoppingList> read_all() override;

    std::vector<std::string> get_all_list_ids() override;

private:
    sqlite3* db = nullptr;

    bool create_schema();
};

#endif
