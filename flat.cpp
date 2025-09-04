#include <bits/stdc++.h>
#include <x86intrin.h>
//Main Assumption here is that we are not adding elements at run time to the map.
static const int TCOUNT = 1e6;
static const int MAXX = 1e6;
static const int INVCOUNT = 1e4;
static inline uint64_t ticks() { _mm_lfence(); auto t=__rdtsc(); _mm_lfence(); return t; }
struct smallstring {
    uint64_t hash;
    uint8_t len; 
    char    buf[8];

    static inline uint64_t compute_hash(uint64_t x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x>>30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x>>27)) * 0x94d049bb133111ebULL;
        return x ^ (x>>31);
    }

    smallstring() noexcept : hash(0), len(0), buf{} {}

    explicit smallstring(std::string_view s) noexcept {
        len = (uint8_t)std::min<size_t>(s.size(), 8);
        std::memset(buf, 0, 8);
        std::memcpy(buf, s.data(), len);
        uint64_t x = 0;
        for (size_t i = 0; i < len; i++) {
            x |= (uint64_t)(uint8_t)buf[i] << (i*8);
        }
        hash = compute_hash(x ^ len);
    }

    std::string_view get_string() const noexcept {
        return std::string_view(buf, len);
    }
};
struct smallstringHash {
    size_t operator()(const smallstring& k) const noexcept {
        return k.hash;
    }
};
struct smallstringEq {
    bool operator()(const smallstring& a, const smallstring& b) const noexcept {
        if (a.hash != b.hash || a.len != b.len) return false;
        return std::memcmp(a.buf, b.buf, a.len) == 0;
    }
};

template <typename K, typename V, typename Hash = std::hash<K>, typename Eq = std::equal_to<K>>
struct flat_map {
    uint64_t mask;
    std::vector<std::pair<K, V>> memory;
    std::vector<uint64_t> prefix_sum;
    Hash hasher;
    Eq eqer;
    flat_map (std::vector<std::pair<K, V>>& elements) {
        size_t N = std::max<size_t>(1, elements.size());
        size_t bit_length = std::bit_width(N - 1);
        size_t bucket_size = (1ll << bit_length);
        mask = bucket_size - 1;
        std::vector<size_t> counter(bucket_size);
        prefix_sum.assign(bucket_size + 1, 0);
        for (auto& [k, v] : elements) {
            uint64_t index = hasher(k) & mask;
            counter[index]++;
        }
        for (size_t i = 0; i < bucket_size; i++) {
            prefix_sum[i+1] = prefix_sum[i] + counter[i];
        }
        std::fill(counter.begin(), counter.end(), 0);
        memory.resize(N);
        for (auto& [k, v] : elements) {
            uint64_t index = hasher(k) & mask;
            size_t next_index = prefix_sum[index] + counter[index];
            counter[index]++;
            memory[next_index] = {k, v};
        }
    }

    const V find (const K& key) {
        uint64_t index = hasher(key) & mask;
        for (size_t i = prefix_sum[index]; i < prefix_sum[index+1]; i++) {
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
    std::set<uint64_t> non_repetitive_set;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    // We have MAXX count of valid keys.
    // We keep INVCOUNT number of elements as non-existing keys.
    std::vector<std::pair<smallstring, int>> map_entries;
    map_entries.resize(MAXX);
    std::vector<smallstring> map_entries_invalid;
    map_entries_invalid.resize(INVCOUNT);

    std::uniform_int_distribution<size_t> len_dist(1, 8);
    std::uniform_int_distribution<char> char_dist('A', 'Z');
    size_t cnt = 0;
    while (non_repetitive_set.size() < MAXX + INVCOUNT) {
        size_t len = len_dist(rng);
        std::string st;
        st.resize(len);
        for (size_t i = 0; i < len; i++) {
            st[i] = char_dist(rng);    
        }
        smallstring sm(st);
        if (non_repetitive_set.find(sm.hash) == non_repetitive_set.end()) {
            if (non_repetitive_set.size() < MAXX) {
                size_t target_index = non_repetitive_set.size();
                map_entries[target_index] = {sm, ++cnt};
            } else {
                size_t target_index = non_repetitive_set.size() - MAXX;
                map_entries_invalid[target_index] = sm;
            }
            non_repetitive_set.insert(sm.hash);
        }
    }
    return {map_entries, map_entries_invalid};
}
int test_correctness(std::vector<std::pair<smallstring, int>>& elements, std::vector<smallstring>& bad_keys) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::cout << "Test Correctness Start!" << std::endl;
    flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(elements);
    bool fail = 0;
    std::vector<int> valid_tests(TCOUNT);
    std::vector<int> invalid_tests(INVCOUNT);
    for (size_t i = 0; i < TCOUNT; i++) {
        valid_tests[i] = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
    }
    
    for (size_t i = 0; i < INVCOUNT; i++) {
        invalid_tests[i] = std::uniform_int_distribution<int>(0, INVCOUNT - 1)(rng);
    }

    for (size_t i = 0; i < TCOUNT; i++) {
        std::string_view symb = elements[valid_tests[i]].first.get_string();
        smallstring sm(symb);
        int res = fmap.find(sm);
        if (res != elements[valid_tests[i]].second) {
            fail = 1;
            std::cout << res << " " << elements[valid_tests[i]].second << "\n";
            break;
        }
    }
    
    for (size_t i = 0; i < INVCOUNT; i++) {
        std::string_view symb = bad_keys[invalid_tests[i]].get_string();
        smallstring sm(symb);
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

void time_it(std::vector<uint64_t>& times, std::ofstream &output_file) {
    std::sort(times.begin(), times.end());
    size_t times_size = times.size();
    std::function <size_t(int)> getKth = [times_size] (int K) {
        return times_size / 100 * K - 1;
    };
    size_t p50 = getKth(50);
    size_t p75 = getKth(75);
    size_t p99 = getKth(99);
    output_file << "p50 Time = " << times[p50] << " cycles , p75 Time = " << times[p75] << " cycles, p99 Time = " << times[p99] << " cycles. \n";
    output_file << "Average Time = " << ((double) std::accumulate(times.begin(), times.end(), (uint64_t) 0) / times_size) << " cycles. \n";
}

int test_time(std::vector<std::pair<smallstring, int>>& elements, std::vector<smallstring>& bad_keys) {
    std::ofstream output_file("test_file.txt");

    if (!output_file) {
        std::cerr << "Error opening file for writing\n";
        return 1;
    }

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    std::cout << "Test Time Start!" << std::endl;
    flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(elements);

    std::vector<int> valid_tests(TCOUNT);
    std::vector<int> invalid_tests(INVCOUNT);
    for (size_t i = 0; i < TCOUNT; i++) {
        valid_tests[i] = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
    }

    for (size_t i = 0; i < INVCOUNT; i++) {
        invalid_tests[i] = std::uniform_int_distribution<int>(0, INVCOUNT - 1)(rng);
    }

    std::vector<uint64_t> times_fmap_valid(TCOUNT);
    std::vector<uint64_t> times_fmap_invalid(INVCOUNT);
     for (size_t i = 0; i < TCOUNT; i++) {
        std::string_view symb = elements[valid_tests[i]].first.get_string();
        smallstring sm(symb);
        uint64_t t1 = ticks();
        int res = fmap.find(sm);
        uint64_t t2 = ticks();
        times_fmap_valid[i] = t2 - t1;
    }
    for (size_t i = 0; i < INVCOUNT; i++) {
        std::string_view symb = bad_keys[invalid_tests[i]].get_string();
        smallstring sm(symb);
        uint64_t t1 = ticks();
        int res = fmap.find(sm);
        uint64_t t2 = ticks();
        times_fmap_invalid[i] = t2 - t1;
    }
    output_file << "---------For Flat_Unordered_map_valid ---------- \n";
    time_it(times_fmap_valid, output_file);

    output_file << "---------For Flat_Unordered_map_invalid ---------- \n";
    time_it(times_fmap_invalid, output_file);
    
    std::unordered_map<smallstring, int, smallstringHash, smallstringEq> umap;
    uint8_t z = std::bit_width((unsigned long long)MAXX);
    umap.max_load_factor(1.0f);
    umap.reserve(MAXX);
    umap.rehash(1ll<<z);

    for (auto& [k, v] : elements) {
        umap[k] = v;
    }

    std::vector<uint64_t> times_umap_valid(TCOUNT);
    std::vector<uint64_t> times_umap_invalid(INVCOUNT);
    for (size_t i = 0; i < TCOUNT; i++) {
        std::string_view symb = elements[valid_tests[i]].first.get_string();
        smallstring sm(symb);
        uint64_t time_start = ticks();
        auto res = umap.find(sm);
        uint64_t time_end = ticks();
        times_umap_valid[i] = time_end - time_start;
    }

    for (size_t i = 0; i < INVCOUNT; i++) {
        std::string_view symb = bad_keys[invalid_tests[i]].get_string();
        smallstring sm(symb);
        uint64_t time_start = ticks();
        auto res = umap.find(sm);
        uint64_t time_end = ticks();
        times_umap_invalid[i] = time_end - time_start;
    }

    output_file << "---------For Flat_Unordered_map_valid ---------- \n";
    time_it(times_umap_valid, output_file);
    
    output_file << "---------For Flat_Unordered_map_invalid ---------- \n";
    time_it(times_umap_invalid, output_file);
    
    std::cout << "Test Time results written to file! \n";
    return 0;
}


int main() {
    auto [elements, bad_keys] = gen_test();
    test_correctness(elements, bad_keys);
    test_time(elements, bad_keys);

    return 0;
}