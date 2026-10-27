#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/IEnumerator.hpp"
#include "pointers/UniquePtr.hpp"

template <class T>
std::string WriteText(const T& value) {
    std::ostringstream output;
    output << std::setprecision(17) << value;
    if (!output) throw std::invalid_argument("Cannot write value.");
    return output.str();
}

template <class T>
T ReadText(const std::string& text) {
    std::istringstream input(text);
    T value{};
    if (!(input >> value)) throw std::invalid_argument("Invalid value.");
    input >> std::ws;
    if (!input.eof()) throw std::invalid_argument("Unexpected text after value.");
    return value;
}

template <>
inline std::string ReadText<std::string>(const std::string& text) {
    return text;
}

template <class T, class Writer>
std::string WriteValues(UniquePtr<IEnumerator<T>> enumerator, Writer write) {
    std::ostringstream output;
    bool first = true;
    while (enumerator->MoveNext()) {
        if (!first) output << ' ';
        first = false;
        const std::string value = write(enumerator->CurrentRef());
        if (value.empty() || value.find_first_of(" \t\n\r\f\v\"\\") != std::string::npos) output << std::quoted(value);
        else output << value;
    }
    return output.str();
}

