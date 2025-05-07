#ifndef HASH_H
#define HASH_H

#include <cctype>
#include <chrono>
#include <random>
#include <string>

typedef std::size_t HASH_INDEX_T;

struct MyStringHash {
    // debug R values
    HASH_INDEX_T rValues[5] {
        983132572ULL,
        1468777056ULL,
        552714139ULL,
        984953261ULL,
        261934300ULL
    };

    MyStringHash(bool debug = true) {
        if (!debug) generateRValues();
    }

    // hash function
    HASH_INDEX_T operator()(const std::string& k) const {
        const int GROUPS = 5;
        const int CHUNK  = 6;
        // pow36[i] = 36^i
        static const unsigned long long pow36[CHUNK] = {
            1ULL,
            36ULL,
            36ULL * 36ULL,
            36ULL * 36ULL * 36ULL,
            36ULL * 36ULL * 36ULL * 36ULL,
            36ULL * 36ULL * 36ULL * 36ULL * 36ULL
        };

        unsigned long long w[GROUPS] = {0,0,0,0,0};
        int n = (int)k.size();

        // for each chunk c=0 (last 6 chars), 1 (prior 6), ..., store in w[4-c]
        for (int c = 0; c < GROUPS; ++c) {
            int end   = n - c*CHUNK;
            if (end <= 0) break;
            int start = end - CHUNK;
            if (start < 0) start = 0;

            unsigned long long chunkVal = 0;
            for (int pos = start; pos < end; ++pos) {
                int idx = (end - 1) - pos;               // power index 0..5
                HASH_INDEX_T v = letterDigitToNumber(k[pos]);
                chunkVal += v * pow36[idx];
            }
            w[GROUPS - 1 - c] = chunkVal;
        }

        unsigned long long h = 0;
        for (int i = 0; i < GROUPS; ++i) {
            h += (unsigned long long)rValues[i] * w[i];
        }
        return (HASH_INDEX_T)h;
    }

    // map 'a'–'z'→0–25, '0'–'9'→26–35
    HASH_INDEX_T letterDigitToNumber(char ch) const {
        unsigned char uc = static_cast<unsigned char>(ch);
        char c = static_cast<char>(std::tolower(uc));
        if (c >= 'a' && c <= 'z') return c - 'a';
        if (c >= '0' && c <= '9') return 26 + (c - '0');
        return 0;
    }

    // generate random rValues
    void generateRValues() {
        unsigned seed = (unsigned)std::chrono::system_clock::now()
                                 .time_since_epoch().count();
        std::mt19937 gen(seed);
        for (int i = 0; i < 5; ++i) {
            rValues[i] = gen();
        }
    }
};

#endif