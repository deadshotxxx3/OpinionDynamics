#include "od/io/InputHelpers.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace od::io {

namespace {

template <typename T, typename Converter>
std::optional<T> parseWhole(const std::string& text, Converter convert) {
    std::string value = trim(text);
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        size_t consumed = 0;
        T result = convert(value, &consumed);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

template <typename T, typename Parser>
std::optional<std::vector<T>> parseList(const std::string& csv, Parser parseItem) {
    std::vector<T> result;
    std::stringstream stream(csv);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (trim(item).empty()) {
            continue;
        }
        std::optional<T> value = parseItem(item);
        if (!value.has_value()) {
            return std::nullopt;
        }
        result.push_back(*value);
    }
    return result;
}

} // namespace

std::string trim(const std::string& s) {
    auto begin = s.begin();
    while (begin != s.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    auto end = s.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
}

std::optional<int> parseInt(const std::string& text) {
    return parseWhole<int>(text, [](const std::string& s, size_t* pos) { return std::stoi(s, pos); });
}

std::optional<double> parseDouble(const std::string& text) {
    return parseWhole<double>(text, [](const std::string& s, size_t* pos) { return std::stod(s, pos); });
}

std::optional<std::vector<int>> parseIntList(const std::string& csv) {
    return parseList<int>(csv, parseInt);
}

std::optional<std::vector<double>> parseDoubleList(const std::string& csv) {
    return parseList<double>(csv, parseDouble);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("ввод закончился раньше, чем ожидалось");
    }
    return trim(line);
}

std::optional<int> readInt(const std::string& prompt) {
    std::string line = readLine(prompt);
    std::optional<int> value = parseInt(line);
    if (!value.has_value()) {
        std::cerr << "Некорректное целое число: '" << line << "'\n";
    }
    return value;
}

std::optional<double> readDouble(const std::string& prompt) {
    std::string line = readLine(prompt);
    std::optional<double> value = parseDouble(line);
    if (!value.has_value()) {
        std::cerr << "Некорректное число: '" << line << "'\n";
    }
    return value;
}

std::optional<int> readIntInRange(const std::string& prompt, int minVal, int maxVal) {
    std::optional<int> value = readInt(prompt);
    if (value.has_value() && (*value < minVal || *value > maxVal)) {
        std::cerr << "Значение должно быть от " << minVal << " до " << maxVal << "\n";
        return std::nullopt;
    }
    return value;
}

std::optional<double> readDoubleInRange(const std::string& prompt, double minVal, double maxVal) {
    std::optional<double> value = readDouble(prompt);
    if (value.has_value() && (*value < minVal || *value > maxVal)) {
        std::cerr << "Значение должно быть от " << minVal << " до " << maxVal << "\n";
        return std::nullopt;
    }
    return value;
}

int readIntUntilValid(const std::string& prompt, int minVal, int maxVal) {
    while (true) {
        std::optional<int> value = readIntInRange(prompt, minVal, maxVal);
        if (value.has_value()) {
            return *value;
        }
    }
}

double readDoubleUntilValid(const std::string& prompt, double minVal, double maxVal) {
    while (true) {
        std::optional<double> value = readDoubleInRange(prompt, minVal, maxVal);
        if (value.has_value()) {
            return *value;
        }
    }
}

bool readYesNo(const std::string& prompt) {
    std::string line = readLine(prompt);
    while (line != "y" && line != "n") {
        line = readLine("Некорректный ввод. Напишите y/n: ");
    }
    return line == "y";
}

} // namespace od::io
