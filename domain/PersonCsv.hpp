#pragma once

#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "dictionaries/HashTable.hpp"
#include "domain/Person.hpp"
#include "pointers/PointerOperations.hpp"

template <class Add>
void ReadPeopleCsv(std::istream& input, Add add) {
    int lineNumber = 1;
    try {
        std::string line;
        std::getline(input, line);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line != "id,firstName,lastName,birthYear") {
            throw std::invalid_argument("Ожидается заголовок id,firstName,lastName,birthYear.");
        }
        HashTable<int, bool> ids;
        while (std::getline(input, line)) {
            ++lineNumber;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            std::istringstream row(line);
            Person person;
            char separator = 0;
            if (line.find('"') != std::string::npos
                || !(row >> person.id) || !row.get(separator) || separator != ','
                || !std::getline(row, person.firstName, ',')
                || !std::getline(row, person.lastName, ',')
                || !(row >> person.birthYear) || !(row >> std::ws).eof()) {
                throw std::invalid_argument("Неверная строка CSV.");
            }
            if (ids.ContainsKey(person.id)) throw std::invalid_argument("Повторяется ID " + std::to_string(person.id));
            ids.Add(person.id, true);
            add(Move(person));
        }
        if (!input.eof()) throw std::runtime_error("Не удалось прочитать CSV.");
    } catch (const std::invalid_argument& error) {
        throw std::invalid_argument("CSV, строка " + std::to_string(lineNumber) + ": " + error.what());
    }
}

template <class Add>
void ReadPeopleCsv(const std::string& path, Add add) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Не удалось открыть CSV: " + path);
    ReadPeopleCsv(input, add);
}
