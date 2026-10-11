#pragma once

template <class T>
T&& Move(T& value) noexcept {
    return static_cast<T&&>(value);
}

template <class T>
void Swap(T& first, T& second) noexcept {
    T temporary = Move(first);
    first = Move(second);
    second = Move(temporary);
}
