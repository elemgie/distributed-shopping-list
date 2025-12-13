#include <string>
#include <chrono>
#include <random>

class Util {
    public:
        static size_t getHash(const std::string &str) {
            size_t hash = 0;
            for (char c : str) {
                hash = (hash * BASE + c) % MOD;
            }
            return hash;
        }

        static uint64_t now_ms() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }

        static int rand_int(int min, int max) {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(min, max);
            return dist(rng);
        }

    private:
        static const long long MOD = 1e9 + 7;
        static const long long BASE = 31;
};