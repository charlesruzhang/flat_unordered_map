#pragma once
#include <bits/stdc++.h>
#include <string_view>
class smallstring {
    public:
        uint64_t hash;
        uint8_t len; 
        char    buf[8];
        smallstring() noexcept : hash(0), len(0), buf{} {}

        explicit smallstring(std::string_view s) noexcept {
            len = s.size() >= 8 ? 8 : static_cast<uint8_t>(s.size());
            memset(buf, 0, 8);
            memcpy(buf, s.data(), len);
            hash = compute_hash(buf, len);
        }

        std::string_view get_string() const noexcept {
            return std::string_view(buf, len);
        }

    private:
        static inline uint64_t compute_hash(char* buf, uint8_t len) noexcept {
            uint64_t x = 0;
            for (size_t i = 0; i < len; i++) {
                x |= (uint64_t)buf[i] << (i*8);
            }
            x ^= len;
            x += 0x9e3779b97f4a7c15ULL;
            x = (x ^ (x>>30)) * 0xbf58476d1ce4e5b9ULL;
            x = (x ^ (x>>27)) * 0x94d049bb133111ebULL;
            return x ^ (x>>31);
        }
};
class smallstringHash {
    public:
        size_t operator()(const smallstring& s) const noexcept {
            return s.hash;
        }
};
class smallstringEq {
    public:
        bool operator()(const smallstring& s, const smallstring& t) const noexcept {
            if (s.hash != t.hash || s.len != t.len) return false;
            return std::memcmp(s.buf, t.buf, t.len) == 0;
        }
};