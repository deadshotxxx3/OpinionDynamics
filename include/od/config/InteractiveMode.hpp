#pragma once

#include "od/config/RuntimeParams.hpp"

namespace od::config {

/**
 * @brief Готовит запуск, задавая вопросы в консоли.
 *
 * Сначала спрашивает, сгенерировать граф или загрузить его из папки прогона, затем
 * собирает параметры. При загрузке предлагает взять параметры из файла или ввести свои.
 *
 * @throws std::invalid_argument Если собранная конфигурация графа некорректна.
 * @throws std::runtime_error Если файл графа не удалось прочитать или ввод закончился.
 */
SimulationSetup setupInteractive();

} // namespace od::config
