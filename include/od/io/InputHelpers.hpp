#pragma once

#include <string>
#include <optional>

namespace od::io{
    std::string trim(const std::string& s);

    std::optional<int> readInt(const std::string& prompt);

    std::optional<double> readDouble(const std::string& prompt);

    std::optional<int> readIntInRange(const std::string& prompt, int MinVal, int MaxVal);

    std::optional<double> readDoubleInRange(const std::string& prompt, double MinVal, double MaxVal);

    bool readYesNo(const std::string& prompt);
}