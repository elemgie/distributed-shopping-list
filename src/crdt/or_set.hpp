#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <msgpack.hpp>

using namespace std;

template <typename T>
class ORSet {
private:
    unordered_map<string, T> elements; // TOOD: rethink if this is really needed
    unordered_map<string, unordered_set<string>> addSet;
    unordered_map<string, unordered_set<string>> removeSet;

    string genTag() const {
        using namespace chrono;
        auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
        return to_string(now);
    }

public:
    ORSet() = default;

    bool contains(const T &elem) const {
        return contains(elem.getUid());
    }

    bool contains(const string &uid) const {
        auto it = addSet.find(uid);
        if (it == addSet.end()) return false;

        auto rit = removeSet.find(uid);
        if (rit == removeSet.end()) return true;

        for (auto &tag : it->second)
            if (rit->second.find(tag) == rit->second.end())
                return true;
        return false;
    }

    void add(const T &elem) {
        const string uid = elem.getUid();
        elements[uid] = elem;
        addSet[uid].insert(genTag());
    }

    void remove(const T &elem) {
        const string uid = elem.getUid();
        auto it = addSet.find(uid);
        if (it != addSet.end()) {
            for (auto &tag : it->second)
                removeSet[uid].insert(tag);
        }
    }

    vector<T*> values() {
        vector<T*> elems;
        for (auto &p : addSet) {
            const string &uid = p.first;
            if (contains(uid)) {
                auto it = elements.find(uid);
                if (it != elements.end()) elems.push_back(&it->second);
            }
        }
        return elems;
    }

    vector<const T*> values() const {
        vector<const T*> elems;
        for (auto &p : addSet) {
            const string &uid = p.first;
            if (contains(uid)) {
                auto it = elements.find(uid);
                if (it != elements.end()) elems.push_back(&it->second);
            }
        }
        return elems;
    }

    void merge(const ORSet<T> &other) {
        for (auto &p : other.elements) {
            if (elements.find(p.first) != elements.end())
                elements[p.first] = elements[p.first].merge(p.second);
            else
                elements[p.first] = p.second;
        }

        for (auto &[uid, tags] : other.addSet)
            addSet[uid].insert(tags.begin(), tags.end());

        for (auto &[uid, tags] : other.removeSet)
            removeSet[uid].insert(tags.begin(), tags.end());
    }

    MSGPACK_DEFINE(elements, addSet, removeSet);
};
