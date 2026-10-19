#pragma once

#include "od/config/BatchArgs.hpp"
#include "od/config/RuntimeParams.hpp"

namespace od::config {

/**
 * @brief Готовит запуск по флагам batch-режима: генерирует граф или загружает его из папки прогона.
 *
 * При генерации параметры берутся только из флагов. При загрузке они берутся из секции PARAMS
 * файла, а заданные флаги их переопределяют. Если в файле нет PARAMS, обязательны
 * `--k1`, `--k2` и `--tmax`.
 *
 * @throws UsageError Если указана неизвестная функция вероятности.
 * @throws std::invalid_argument Если параметры несогласованы или конфигурация графа некорректна.
 * @throws std::runtime_error Если файл графа не удалось прочитать.
 */
SimulationSetup setupBatch(const BatchArgs& args);

} // namespace od::config
