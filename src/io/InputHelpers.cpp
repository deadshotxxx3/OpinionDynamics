#include "od/io/InputHelpers.hpp"
#include <iostream>
#include <cctype>


std::string od::io::trim(const std::string& s) {
    if (s.empty()) {
        return s;
    }

    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }

    if (start == s.end()) {
        return "";
    }

    auto end = s.end();
    while (std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    return std::string(start, end);
}


std::optional<int> od::io::readInt(const std::string& prompt){
    std::string line;
    std::cout << prompt;
    std::getline(std::cin,line);
    line = trim(line);

    try{
        size_t pos;
        int validInt = std::stoi(line, &pos);

        if (pos != line.size()){
            std::cerr << "строка " << line << " является некорректным числом\n";
            return std::nullopt;
        }
        return validInt;
    }

    catch (...){
        std::cerr << "Вы ввели некорректное число\n";
        return std::nullopt;
    }
}


std::optional<double> od::io::readDouble(const std::string& prompt){
    std::string line;
    std::cout << prompt;
    std::getline(std::cin,line);
    line = trim(line);

    try{
        size_t pos;
        double validDouble = std::stod(line, &pos);

        if (pos != line.size()){
            std::cerr << "строка " << line << " является некорректным числом\n";
            return std::nullopt;
        }
        return validDouble;
    }

    catch (...){
        std::cerr << "Вы ввели некорректное число\n";
        return std::nullopt;
    }
}


std::optional<int> od::io::readIntInRange(const std::string& prompt, int MinVal, int MaxVal){
    auto val = od::io::readInt(prompt);

    if (val.has_value() && *val >= MinVal && *val <= MaxVal){
        return *val;
    }
    std::cerr << "Значение не в промежутке\n";
    return std::nullopt;
}


std::optional<double> od::io::readDoubleInRange(const std::string& prompt, double MinVal, double MaxVal){
    auto val = od::io::readDouble(prompt);

    if (val.has_value() && *val >= MinVal && *val <= MaxVal){
        return *val;
    }
    std::cerr << "Значение не в промежутке\n";
    return std::nullopt;
}


bool od::io::readYesNo(const std::string& prompt){
    std::string line;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, line);
        line = trim(line);
        if (line == "y" || line == "n") {
            return line == "y";
        }
        std::cout << "Некорректный ввод. Напишите y/n: ";
    }
}