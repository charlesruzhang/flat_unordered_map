#include <bits/stdc++.h>
#include <x86intrin.h>
//Main Assumption here is that we are not adding elements at run time to the map.
static const int TCOUNT = 1e6;
static const int MAXX = 1e6;
static const int INVCOUNT = 1e4;
static constexpr double CPU_FREQ_HZ = 3201000000.0; // CHANGE THIS TO THE REAL FREQUENCY.
static constexpr double CYCLES_TO_NS = 1e9 / CPU_FREQ_HZ;
static inline uint64_t ticks() { _mm_lfence(); auto t=__rdtsc(); _mm_lfence(); return t; }
const std::array<int, 2> seed_prime = {9973, 10007};
struct smallstring {
    uint64_t hash;
    uint64_t sz; 
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
    smallstring() {
        sz = 0;
        s = "";
        hash = 0;
    }
};
struct smallstringHash {
    size_t operator()(const smallstring& k) const noexcept {
        return k.hash;
    }
};
struct smallstringEq {
    bool operator()(const smallstring& a, const smallstring& b) const noexcept {
        return a.hash == b.hash && a.s == b.s;
    }
};
template <typename K, typename V, typename Hash = std::hash<K>, typename Eq = std::equal_to<K>>
struct flat_map {
    std::vector<std::pair<K, V>> memory;
    Hash hasher;
    Eq eqer;
    uint64_t mask;
    std::vector<uint64_t> pref;
    flat_map (std::vector<std::pair<K, V>>& elements) {
        uint64_t N = elements.size();
        int bit_length = std::bit_width(N);
        mask = (1ll << bit_length) - 1;
        std::vector<int> cnt(mask+1);
        pref.resize(mask+2);
        pref[0] = 0;
        for (auto& [k, v] : elements) {
            uint64_t index = hasher(k) & mask;
            cnt[index]++;
        }
        for (int i = 0; i <= mask; i++) {
            pref[i+1] = pref[i] + cnt[i];
            cnt[i] = 0;
        }
        pref[mask+1] = N;
        memory.resize(N);
        for (auto& [k, v] : elements) {
            uint64_t index = hasher(k) & mask;
            memory[pref[index] + cnt[index]] = {k, v};
            cnt[index]++;
        }
    }

    V find (const K& key) {
        uint64_t index = hasher(key) & mask;
        for (int i = pref[index]; i < pref[index+1]; i++) {
            auto& [k, v] = memory[i];
            if (eqer(key, k)) {
                return v;
            }
        }
        // We can use std::optional here, but here we assume that V has a default constructor.
        return V();
    }
};

std::pair<std::vector<std::pair<smallstring, int>>, std::vector<smallstring>> gen_test() {
    std::vector<std::pair<smallstring, int>> elements;
    std::set<uint64_t> s;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    int cnt = 0;
    // We have MAXX count of valid keys.
    // We keep INVCOUNT number of elements as non-existing keys.
    std::vector<smallstring> bad_keys;
    while (s.size() < MAXX + INVCOUNT) {
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
    return {elements, bad_keys};
}
int test_correctness(std::vector<std::pair<smallstring, int>>& elements, std::vector<smallstring>& bad_keys) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::cout << "Test Correctness Start!" << std::endl;
    flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(elements);
    bool fail = 0;
    std::vector<int> valid_tests(TCOUNT);
    std::vector<int> invalid_tests(INVCOUNT);
    for (int i = 0; i < TCOUNT; i++) {
        int index = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
        valid_tests[i] = index;
    }
    
    for (int i = 0; i < INVCOUNT; i++) {
        int index = std::uniform_int_distribution<int>(0, INVCOUNT - 1)(rng);
        invalid_tests[i] = index;
    }

    for (int i = 0; i < TCOUNT; i++) {
        std::string s = elements[valid_tests[i]].first.s;
        smallstring sm(s);
        int res = fmap.find(sm);
        if (res != elements[valid_tests[i]].second) {
            fail = 1;
            break;
        }
    }
    
    for (int i = 0; i < INVCOUNT; i++) {
        std::string s = bad_keys[invalid_tests[i]].s;
        smallstring sm(s);
        int res = fmap.find(sm);
        if (res != 0) {
            fail = 1;
            break;
        }
    }
    if (fail) {
        std::cout << "Test Correctness Failed :(" << std::endl;
        return 1;
    } else {
        std::cout << "Test Correctness Passed :D" << std::endl;
    }
    return 0;
}
void time_it (std::vector<uint64_t>& times) {
    std::sort(times.begin(), times.end());
    int TOTAL = times.size();
    int p50 = TOTAL / 100 * 50 - 1;
    double p50Time = times[p50] * CYCLES_TO_NS;
    int p75 = TOTAL / 100 * 75 - 1;
    double p75Time = times[p75] * CYCLES_TO_NS;
    int p99 = TOTAL / 100 * 99 - 1;
    double p99Time = times[p99] * CYCLES_TO_NS;
    double aveTime =  (double) std::accumulate(times.begin(), times.end(), (uint64_t) 0) / TOTAL * CYCLES_TO_NS;
    std::cout << "p50 Time = " << p50Time << "ns , p75 Time = " << p75Time << "ns, p99 Time = " << p99Time << "ns. " << std::endl;
    std::cout << "Average Time = " << aveTime << "ns." << std::endl;
}

int test_time(std::vector<std::pair<smallstring, int>>& elements, std::vector<smallstring>& bad_keys) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    std::cout << "Test Time Start!" << std::endl;
    flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(elements);
    bool fail = 0;
    std::vector<int> valid_tests(TCOUNT);
    std::vector<int> invalid_tests(INVCOUNT);
    for (int i = 0; i < TCOUNT; i++) {
        int index = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
        valid_tests[i] = index;
    }
    for (int i = 0; i < INVCOUNT; i++) {
        int index = std::uniform_int_distribution<int>(0, INVCOUNT - 1)(rng);
        invalid_tests[i] = index;
    }
    std::vector<uint64_t> times_fmap_valid(TCOUNT);
    std::vector<uint64_t> times_fmap_invalid(INVCOUNT);
     for (int i = 0; i < TCOUNT; i++) {
        std::string symb = elements[valid_tests[i]].first.s;
        smallstring sm(symb);
        uint64_t t1 = ticks();
        int res = fmap.find(sm);
        uint64_t t2 = ticks();
        times_fmap_valid[i] = t2 - t1;
    }
    for (int i = 0; i < INVCOUNT; i++) {
        std::string symb = bad_keys[invalid_tests[i]].s;
        smallstring sm(symb);
        uint64_t t1 = ticks();
        int res = fmap.find(sm);
        uint64_t t2 = ticks();
        times_fmap_invalid[i] = t2 - t1;
    }
    int TOTAL = TCOUNT + INVCOUNT;
    std::cout << "---------For Flat_Unordered_map_valid ----------" << std::endl;
    time_it(times_fmap_valid);

    
    std::cout << "---------For Flat_Unordered_map_invalid ----------" << std::endl;
    time_it(times_fmap_invalid);
    
    
    std::unordered_map<smallstring, int, smallstringHash, smallstringEq> umap;
    umap.max_load_factor(1.0f);
    umap.reserve(MAXX);
    int z = std::bit_width((unsigned long long)MAXX);
    umap.rehash(1ll<<z);

    for (auto& [k, v] : elements) {
        umap[k] = v;
    }
    std::vector<uint64_t> times_umap_valid(TCOUNT);
    std::vector<uint64_t> times_umap_invalid(INVCOUNT);
    for (int i = 0; i < TCOUNT; i++) {
        std::string symb = elements[valid_tests[i]].first.s;
        smallstring sm(symb);
        uint64_t t1 = ticks();
        auto res = umap.find(sm);
        uint64_t t2 = ticks();
        times_umap_valid[i] = t2 - t1;
    }
    for (int i = 0; i < INVCOUNT; i++) {
        std::string symb = bad_keys[invalid_tests[i]].s;
        smallstring sm(symb);
        uint64_t t1 = ticks();
        auto res = umap.find(sm);
        uint64_t t2 = ticks();
        times_umap_invalid[i] = t2 - t1;
    }
    std::cout << "---------For Unordered_map_valid ----------" << std::endl;
    time_it(times_umap_valid);
    
    std::cout << "---------For Unordered_map_invalid ----------" << std::endl;
    time_it(times_umap_invalid);
    return 0;
}


int main() {
    auto [elements, bad_keys] = gen_test();
    test_correctness(elements, bad_keys);
    test_time(elements, bad_keys);

    return 0;
}