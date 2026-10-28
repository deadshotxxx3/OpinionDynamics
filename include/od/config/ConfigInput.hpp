#pragma once

#include "od/config/ManualGenConfig.hpp"

namespace od::config {

/// @brief Способ ввода уровней в интерактивном режиме.
enum class GenerationMode {
    Manual,  ///< Свои число вершин и вероятность для каждого уровня.
    General  ///< Одинаковые число вершин и вероятность для всех уровней.
};

/// @brief Спрашивает способ ввода уровней.
GenerationMode chooseGenerationMode();

/// @brief Собирает конфигурацию генерации, спрашивая параметры каждого уровня отдельно.
ManualGenConfig collectManualGenConfig();

/// @brief Собирает конфигурацию генерации с одинаковыми параметрами всех уровней.
ManualGenConfig collectGeneralGenConfig();

/// @brief Спрашивает, нужна ли динамика рёбер, и функции появления и удаления рёбер.
void collectDynamicEdgesSettings(ManualGenConfig& config);

/**
 * @brief Спрашивает число мнений, функцию изменения мнений, правило выбора, популярность,
 *        начальные мнения и учёт весов.
 * @param config Конфигурация, куда записываются ответы.
 * @param hasStubborn Есть ли в графе упрямые вершины: только тогда спрашивается их начальное мнение.
 */
void collectOpinionModelSettings(ManualGenConfig& config, bool hasStubborn);

} // namespace od::config
