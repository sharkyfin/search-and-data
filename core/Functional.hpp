#pragma once

#include <string>

template <class T>
struct Less {
    constexpr bool operator()(const T& lhs, const T& rhs) const {
        return lhs < rhs;
    }
};

template <class T>
struct Hash {
    int operator()(const T& value) const noexcept {
        const int h = static_cast<int>(value);
        return h >= 0 ? h : -h;
    }
};

template <>
struct Hash<std::string> {
    int operator()(const std::string& key) const noexcept {
        unsigned int hashValue = 2166136261U;
        for (char c : key) {
            hashValue ^= static_cast<unsigned char>(c);
            hashValue *= 16777619U;
        }
        return static_cast<int>(hashValue & 0x7FFFFFFFU);
    }
};
