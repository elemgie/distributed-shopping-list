#include "sqlite_db.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <optional>
#include <msgpack.hpp>
#include <iostream>

using namespace std;

SqliteDb::~SqliteDb() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool SqliteDb::init_db(const string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        cerr << "Failed to open DB: " << sqlite3_errmsg(db) << endl;
        db = nullptr;
        return false;
    }
    return create_schema();
}

bool SqliteDb::create_schema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS lists ("
        "id TEXT PRIMARY KEY, "
        "data BLOB NOT NULL"
        ");";

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg) {
            cerr << "Failed to create schema: " << errmsg << endl;
            sqlite3_free(errmsg);
        }
        return false;
    }
    return true;
}

bool SqliteDb::write(const ShoppingList& list) {
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, list);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT OR REPLACE INTO lists (id, data) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, list.getUid().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, buffer.data(), buffer.size(), SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) cerr << "Write failed: " << sqlite3_errmsg(db) << endl;
    sqlite3_finalize(stmt);
    return ok;
}

optional<ShoppingList> SqliteDb::read(const string& listId) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT data FROM lists WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return nullopt;

    sqlite3_bind_text(stmt, 1, listId.c_str(), -1, SQLITE_TRANSIENT);

    optional<ShoppingList> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob_data = sqlite3_column_blob(stmt, 0);
        int blob_size = sqlite3_column_bytes(stmt, 0);
        if (blob_data && blob_size > 0) {
            msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(blob_data), blob_size);
            ShoppingList list;
            oh.get().convert(list);
            result = move(list);
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

bool SqliteDb::delete_list(const string& listId) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM lists WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, listId.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) cerr << "Delete failed: " << sqlite3_errmsg(db) << endl;
    sqlite3_finalize(stmt);
    return ok;
}

bool SqliteDb::write_many(const vector<ShoppingList>& lists) {
    if (!db) return false;

    char* errmsg = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) cerr << "BEGIN failed: " << errmsg << endl;
        sqlite3_free(errmsg);
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT OR REPLACE INTO lists (id, data) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    bool all_ok = true;
    for (const auto& list : lists) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, list);

        sqlite3_bind_text(stmt, 1, list.getUid().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 2, buffer.data(), buffer.size(), SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            all_ok = false;
            cerr << "Batch write failed for " << list.getUid() << ": " << sqlite3_errmsg(db) << endl;
        }
    }

    sqlite3_finalize(stmt);
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) cerr << "COMMIT failed: " << errmsg << endl;
        sqlite3_free(errmsg);
        return false;
    }

    return all_ok;
}

vector<optional<ShoppingList>> SqliteDb::read_many(const vector<string>& listIds) {
    if (listIds.empty()) return {};

    vector<optional<ShoppingList>> results;
    string sql = "SELECT id, data FROM lists WHERE id IN (";
    for (size_t i = 0; i < listIds.size(); ++i) {
        sql += (i == listIds.size() - 1) ? "?) " : "?, ";
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare batch read: " << sqlite3_errmsg(db) << endl;
        return results;
    }

    for (size_t i = 0; i < listIds.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), listIds[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* uid_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const void* blob_data = sqlite3_column_blob(stmt, 1);
        int blob_size = sqlite3_column_bytes(stmt, 1);

        if (uid_text && blob_data && blob_size > 0) {
            msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(blob_data), blob_size);
            ShoppingList list;
            oh.get().convert(list);
            results.push_back(move(list));
        }
    }

    sqlite3_finalize(stmt);
    return results;
}

bool SqliteDb::delete_many(const vector<string>& listIds) {
    if (!db || listIds.empty()) return false;

    char* errmsg = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) cerr << "BEGIN failed: " << errmsg << endl;
        sqlite3_free(errmsg);
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM lists WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    bool all_ok = true;
    for (const auto& id : listIds) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            all_ok = false;
            cerr << "Batch delete failed for " << id << ": " << sqlite3_errmsg(db) << endl;
        }
    }

    sqlite3_finalize(stmt);
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) cerr << "COMMIT failed: " << errmsg << endl;
        sqlite3_free(errmsg);
        return false;
    }

    return all_ok;
}

vector<ShoppingList> SqliteDb::read_all() {
    vector<ShoppingList> lists;
    const char* sql = "SELECT data FROM lists;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return lists;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob_data = sqlite3_column_blob(stmt, 0);
        int blob_size = sqlite3_column_bytes(stmt, 0);
        if (blob_data && blob_size > 0) {
            msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(blob_data), blob_size);
            ShoppingList list;
            oh.get().convert(list);
            lists.push_back(move(list));
        }
    }

    sqlite3_finalize(stmt);
    return lists;
}

vector<string> SqliteDb::get_all_list_ids() {
    vector<string> ids;
    const char* sql = "SELECT id FROM lists;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return ids;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* uid_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (uid_text) ids.emplace_back(uid_text);
    }

    sqlite3_finalize(stmt);
    return ids;
}
