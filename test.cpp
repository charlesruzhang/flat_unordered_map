#include <bits/stdc++.h>
#include <bit>
#include <string_view>
#include "smallstring.hpp"
#include "flat.hpp"

#if defined(__aarch64__)

inline uint64_t ticks() noexcept {
    uint64_t cntvct;
    asm volatile("isb; mrs %0, cntvct_el0; isb;" : "=r"(cntvct) :: "memory");
    return cntvct;
}

#elif defined(__x86_64__) || defined(_M_X64)

#include <x86intrin.h>

inline uint64_t ticks() noexcept {
    uint64_t tsc;
    _mm_lfence();
    tsc = __rdtsc();
    _mm_lfence();
    return tsc;
}

#else
#error "ticks() not implemented for this architecture"
#endif

class test_flat_map {
    private:
        // We have MAXX count of valid keys.
        // We keep INVCOUNT number of elements as non-existing keys.
        // We run TCOUNT random visits to the map.
        int TCOUNT;
        int MAXX;
        int INVCOUNT;
        
        std::ofstream output_file;
        std::vector<std::pair<smallstring, int>> map_entries;
        std::vector<smallstring> map_entries_invalid;
        std::mt19937 rng;
        void gen_test() {
            std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<size_t> len_dist(1, 8);
            std::uniform_int_distribution<char> char_dist('A', 'Z');

            std::set<uint64_t> non_repetitive_set;
            size_t cnt = 0;
            // Keep adding strings that are unique to the set until we get enough elements.
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
        }

        int test_correctness() {
            std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::cout << "Test Correctness Start!" << std::endl;
            flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(map_entries);
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
                std::string_view symb = map_entries[valid_tests[i]].first.get_string();
                smallstring sm(symb);
                auto res = fmap.find(sm);
                if (!res || (*res) != map_entries[valid_tests[i]].second) {
                    fail = 1;
                    break;
                }
            }
            
            for (size_t i = 0; i < INVCOUNT; i++) {
                std::string_view symb = map_entries_invalid[invalid_tests[i]].get_string();
                smallstring sm(symb);
                auto res = fmap.find(sm);
                if (res) {
                    fail = 1;
                    break;
                }
            }
            if (fail) {
                std::cout << "Test Correctness Failed :(" << std::endl;
                return 1;
            } else {
                std::cout << "Test Correctness Passed :D" << std::endl;
                return 0;
            }
        }

        void time_it(std::vector<uint64_t>& times) {
            std::sort(times.begin(), times.end());
            size_t times_size = times.size();
            auto getKth = [&times](int K)->size_t {
                return std::min(times.size()-1, (K * (times.size()-1) + 50) / 100);
            };

            size_t p50 = getKth(50);
            size_t p75 = getKth(75);
            size_t p99 = getKth(99);
            output_file << "p50 Time = " << times[p50] << " cycles , p75 Time = " << times[p75] << " cycles, p99 Time = " << times[p99] << " cycles. \n";
            output_file << "Average Time = " << ((double) std::accumulate(times.begin(), times.end(), (uint64_t) 0) / times_size) << " cycles. \n";
        }

        template <typename Map>
        void test_time_helper(Map& m, std::vector<int>& valid_tests, std::vector<int>& invalid_tests, std::string target_string) {
            std::vector<uint64_t> times_map_valid(TCOUNT);
            std::vector<uint64_t> times_map_invalid(INVCOUNT);
            for (size_t i = 0; i < TCOUNT; i++) {
                std::string_view symb = map_entries[valid_tests[i]].first.get_string();
                smallstring sm(symb);
                uint64_t t1 = ticks();
                m.find(sm);
                uint64_t t2 = ticks();
                times_map_valid[i] = t2 - t1;
            }
            for (size_t i = 0; i < INVCOUNT; i++) {
                std::string_view symb = map_entries_invalid[invalid_tests[i]].get_string();
                smallstring sm(symb);
                uint64_t t1 = ticks();
                m.find(sm);
                uint64_t t2 = ticks();
                times_map_invalid[i] = t2 - t1;
            }
            output_file << "---------For " << target_string << "_valid ---------- \n";
            time_it(times_map_valid);

            output_file << "---------For " << target_string << "_invalid ---------- \n";
            time_it(times_map_invalid);
        }

        void test_time() {
            std::cout << "Test Time Start!" << std::endl;
            std::vector<int> valid_tests(TCOUNT);
            std::vector<int> invalid_tests(INVCOUNT);
            for (size_t i = 0; i < TCOUNT; i++) {
                valid_tests[i] = std::uniform_int_distribution<int>(0, MAXX - 1)(rng);
            }

            for (size_t i = 0; i < INVCOUNT; i++) {
                invalid_tests[i] = std::uniform_int_distribution<int>(0, INVCOUNT - 1)(rng);
            }
            
            flat_map<smallstring, int, smallstringHash, smallstringEq> fmap(map_entries);
            test_time_helper(fmap, valid_tests, invalid_tests, std::string("flat_map"));
            
            std::unordered_map<smallstring, int, smallstringHash, smallstringEq> umap;
            uint8_t bit_len = std::bit_width((uint64_t)MAXX);
            umap.max_load_factor(1.0f);
            umap.reserve(MAXX);
            umap.rehash(1ll<<bit_len);

            for (auto& [k, v] : map_entries) {
                umap[k] = v;
            }
            test_time_helper(umap, valid_tests, invalid_tests, std::string("unordered_map"));
            
            std::cout << "Test Time results written to file! \n";
        }
    
    public: 
        test_flat_map (int tcount, int maxx, int invcount) {
            rng = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
            TCOUNT = tcount;
            MAXX = maxx;
            INVCOUNT = invcount;
        }

        int init(const std::string file_location) {
            output_file = std::ofstream(file_location);
            if (!output_file) {
                std::cerr << "Error opening file for writing\n";
                return 1;
            }
            map_entries.resize(MAXX);
            map_entries_invalid.resize(INVCOUNT);
            gen_test();
            return 0;
        }

        void test() {
            if (test_correctness()) return;
            test_time();
        }
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <tcount> <maxx> <invcount> <output_file>\n";
        return 1;
    }
    static_assert(sizeof(smallstring) == 16);
    size_t tcount   = std::stoull(argv[1]);
    size_t maxx     = std::stoull(argv[2]);
    size_t invcount = std::stoull(argv[3]);
    std::string out = argv[4];

    test_flat_map Test(tcount, maxx, invcount);
    if (Test.init(out)) return 1;
    Test.test();

    return 0;
}