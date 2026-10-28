#pragma once

#include <optional>
#include <string>
#include <vector>

/// @brief Ввод-вывод: чтение пользовательского ввода, файлы графа и лога.
namespace od::io {

/// @brief Убирает пробельные символы в начале и в конце строки.
std::string trim(const std::string& s);

/// @brief Разбирает целое число. Вся строка, кроме пробелов по краям, должна быть числом.
/// @return Число или `std::nullopt`, если строка не является целым числом.
std::optional<int> parseInt(const std::string& text);

/// @brief Разбирает вещественное число. Вся строка, кроме пробелов по краям, должна быть числом.
/// @return Число или `std::nullopt`, если строка не является числом.
std::optional<double> parseDouble(const std::string& text);

/// @brief Разбирает seed: целое от 0 до `UINT_MAX` без знака.
/// @return Seed или `std::nullopt`, если строка не подходит.
std::optional<unsigned int> parseSeed(const std::string& text);

/// @brief Разбирает список целых через запятую, пустые элементы пропускаются.
/// @return Список или `std::nullopt`, если хотя бы один элемент не является целым числом.
std::optional<std::vector<int>> parseIntList(const std::string& csv);

/// @brief Разбирает список чисел через запятую, пустые элементы пропускаются.
/// @return Список или `std::nullopt`, если хотя бы один элемент не является числом.
std::optional<std::vector<double>> parseDoubleList(const std::string& csv);

/**
 * @brief Печатает `prompt` и читает строку ввода без пробелов по краям.
 * @throws std::runtime_error Если ввод закончился (конец файла или потока).
 */
std::string readLine(const std::string& prompt);

/// @brief Печатает `prompt` и читает целое число.
/// @return Число или `std::nullopt` с сообщением об ошибке в `std::cerr`.
std::optional<int> readInt(const std::string& prompt);

/// @brief Печатает `prompt` и читает вещественное число.
/// @return Число или `std::nullopt` с сообщением об ошибке в `std::cerr`.
std::optional<double> readDouble(const std::string& prompt);

/// @brief Как `readInt`, но дополнительно проверяет, что число лежит в `[minVal, maxVal]`.
std::optional<int> readIntInRange(const std::string& prompt, int minVal, int maxVal);

/// @brief Как `readDouble`, но дополнительно проверяет, что число лежит в `[minVal, maxVal]`.
std::optional<double> readDoubleInRange(const std::string& prompt, double minVal, double maxVal);

/// @brief Спрашивает целое из `[minVal, maxVal]`, пока пользователь не введёт корректное.
int readIntUntilValid(const std::string& prompt, int minVal, int maxVal);

/// @brief Спрашивает число из `[minVal, maxVal]`, пока пользователь не введёт корректное.
double readDoubleUntilValid(const std::string& prompt, double minVal, double maxVal);

/// @brief Спрашивает seed (от 0 до `UINT_MAX`), пока пользователь не введёт корректный.
unsigned int readSeedUntilValid(const std::string& prompt);

/// @brief Спрашивает `y` или `n`, пока не получит один из них.
/// @return `true` для `y`, `false` для `n`.
bool readYesNo(const std::string& prompt);

} // namespace od::io
