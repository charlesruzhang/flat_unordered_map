#pragma once
#include <bits/stdc++.h>
#include <bit>
#include <string_view>
#include <x86intrin.h>
#include "smallstring.hpp"
//Main Assumption here is that we are not adding elements at run time to the map.
template <typename K, typename V, typename Hash = std::hash<K>, typename Eq = std::equal_to<K>>
class alignas(64) flat_map {
    private:
        uint64_t mask;
        std::vector<std::pair<K, V>> memory;
        std::vector<uint64_t> prefix_sum;
        Hash hasher;
        Eq eqer;
    
    public:
        flat_map (std::vector<std::pair<K, V>>& elements) {
            uint64_t N = elements.size();
            
            memory.resize(N);
            size_t bit_len = std::bit_width(N - 1);
            uint64_t bucket_size = 1ULL << bit_len;
            mask = bucket_size - 1;
            std::vector<size_t> counter(bucket_size);
            prefix_sum.assign(bucket_size + 1, 0);

            // Count the size of each bucket.
            for (auto& [k, v] : elements) {
                uint64_t index = hasher(k) & mask;
                counter[index]++;
            }
            // Store the prefix-sum-count in the prefix-sum array for future O(1) queries.
            for (size_t i = 0; i < bucket_size; i++) {
                prefix_sum[i+1] = prefix_sum[i] + counter[i];
            }
            std::fill(counter.begin(), counter.end(), 0);
            for (auto& [k, v] : elements) {
                uint64_t index = hasher(k) & mask;
                size_t next_index = prefix_sum[index] + counter[index];
                counter[index]++;
                memory[next_index] = {k, v};
            }
        }

        std::optional<std::reference_wrapper<V>> find(const K& key) {
            uint64_t index = hasher(key) & mask;
            uint64_t begin = prefix_sum[index];
            uint64_t end = prefix_sum[index+1];
            for (size_t i = begin; i < end; i++) {
                auto& [k, v] = memory[i];
                if (eqer(key, k)) {
                    return v;
                }
            }
            return std::nullopt;
        }
};