#ifndef PN_COUNTER_HPP
#define PN_COUNTER_HPP

#include <unordered_map>
#include <string>
#include <msgpack.hpp>

using namespace std;

class PNCounter {
private:
    unordered_map<string, uint64_t> pos, neg;

public:
    PNCounter() = default;

    void increment(const string& origin, int64_t delta = 1) {
        if (delta > 0) {
            pos[origin] += delta;
        }
    }

    void decrement(const string& origin, int64_t delta = 1) {
        if (delta > 0) {
            neg[origin] += delta;
        }
    }

    int64_t value() const {
        int64_t pos_sum = 0;
        for (const auto& [origin, count]: pos) {
            pos_sum += count;
        }

        int64_t neg_sum = 0;
        for (const auto& [origin, count] : neg) {
            neg_sum += count;
        }

        return pos_sum - neg_sum;
    }

    void merge(const PNCounter& other) {
        for (const auto& [origin, count] : other.pos) {
            pos[origin] = max(pos[origin], count);
        }
        for (const auto& [origin, count] : other.neg) {
            neg[origin] = max(neg[origin], count);
        }
    }

    MSGPACK_DEFINE(pos, neg);
};

#endif