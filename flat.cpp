#include <bits/stdc++.h>
//Main Assumption here is that we are not adding elements at run time to the map.
struct smallstring {
    uint64_t hash;
    uint64_t sz; 
    const std::array<int, 2> seed_prime = {9973, 10007};
    std::string s;
    smallstring(const std::string& symbol) {
        sz = symbol.size();
        s = symbol;
        std::array<uint64_t, 2> hashval = {0, 0};
        for (int i = 0; i < sz; i++) {
            for (int j = 0; j < 2; j++) {
                hashval[j] *= seed_prime[j];
                hashval[j] += symbol[i];
            }
        }
        hashval[0] ^= (hashval[0] >> 17);
        hashval[1] ^= (hashval[1] >> 27);
        hash = hashval[0] * 0xbf58476d1ce4e5b9ULL + hashval[1];
    }
};
struct smallstringHash {
    size_t operator()(const smallstring& k) const noexcept {
        return k.hash;
    }
};
struct smallstringEq {
    bool operator()(const smallstring& a, const smallstring& b) const noexcept {
        return a.hash == b.hash;
    }
};
template <typename K, typename V, typename Hash = std::hash<K>, typename Eq = std::equal_to<K>>
struct flat_map {
    std::vector<std::vector<std::pair<K, V>>> memory;
    Hash hasher;
    Eq eqer;
    uint64_t mask;
    flat_map (std::vector<std::pair<K, V>>& elements) {
        uint64_t N = elements.size();
        int bit_length = std::bit_width(N);
        memory.resize(1ll << bit_length);
        mask = (1ll << bit_length) - 1;
        std::cout << mask << std::endl;
        for (auto [k, v] : elements) {
            uint64_t index = hasher(k) & mask;
            memory[index].push_back({k, v});
        }
    }

    V find (K& key) {
        uint64_t index = hasher(key) & mask;
        for (auto [k, v] : memory[index]) {
            if (eqer(key, k)) {
                return v;
            }
        }
        // We can use std::optional here, this assumes that V has a default constructor.
        return V();
    }
};


int test1() {
    std::vector<std::pair<smallstring, int>> elements;
    std::set<uint64_t> s;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    size_t MAXX = 1e6;
    int cnt = 0;
    // We keep offset number of elements as non-existing keys;
    int offset = 100;
    std::vector<smallstring> bad_keys;
    while (s.size() < MAXX + offset) {
        int len = std::uniform_int_distribution<int>(1, 8)(rng);
        std::string st;
        for (int i = 0; i < len; i++) {
            char c = (char) ('A' + std::uniform_int_distribution<int>(0, 25)(rng));
            st += c;           
        }
        smallstring sm(st);
        if (s.find(sm.hash) == s.end()) {
            if (s.size() < MAXX) {
                elements.push_back({sm, ++cnt});
            } else {
                bad_keys.push_back(sm);
            }
            s.insert(sm.hash);
        }
    }
    std::cout << "Test Start!" << std::endl;
    flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(elements);
    bool fail = 0;
    for (int i = 0; i < MAXX; i++) {
        int index = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
        int res = fmap.find(elements[index].first);
        if (res != elements[index].second) {
            fail = 1;
            break;
        }
    }
    for (int i = 0; i < offset; i++) {
        int index = std::uniform_int_distribution<int>(0, offset - 1)(rng);
        int res = fmap.find(bad_keys[index]);
        if (res != 0) {
            fail = 1;
            break;
        }
    }
    if (fail) {
        std::cout << "Test Failed :(" << std::endl;
    } else {
        std::cout << "Test Passed :D" << std::endl;
    }
    // TODO: timer, readme.
    //std::unordered_map<smallstring, int, smallstringHash> umap;
    return 0;
}

int main() {
    test1();
    return 0;
}