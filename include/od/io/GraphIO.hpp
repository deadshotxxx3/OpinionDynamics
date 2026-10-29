#pragma once

#include "od/graph/Graph.hpp"
#include "od/model/InitialOpinions.hpp"
#include "od/model/OpinionFunction.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace od::io {

/**
 * @brief Параметры прогона в том виде, как они хранятся в секции PARAMS.
 *
 * Значения по умолчанию — это то, что подразумевают файлы, сохранённые до появления
 * соответствующей строки: два мнения, правило «пропорционально», обычные вершины
 * начинают с 0, упрямые с 1.
 */
struct SimulationParams {
    unsigned int seed = 0;                             ///< `SEED`
    int tMax = 0;                                      ///< `T_MAX`
    int numOpinions = od::model::DEFAULT_NUM_OPINIONS; ///< `NUM_OPINIONS`
    od::model::OpinionFunction opinionFunction = od::model::defaultOpinionFunction(); ///< `OPINION_FUNCTION`
    od::model::ChoiceRule choiceRule = od::model::ChoiceRule::Proportional;          ///< `CHOICE_RULE`
    std::vector<int> popularity;                       ///< `POPULARITY`
    od::model::InitialOpinionSettings initialOpinions; ///< `INITIAL_REGULAR`, `INITIAL_STUBBORN`, `INITIAL_MANUAL`
    bool useWeights = false;                           ///< `USE_WEIGHTS`
    bool dynamicEdges = false;                         ///< `DYNAMIC_EDGES`
    od::model::ProbabilityFunction edgeAddFunction;    ///< `ADD_FUNCTION`
    bool removeEdges = false;                          ///< `REMOVE_EDGES`
    od::model::ProbabilityFunction edgeRemoveFunction; ///< `REMOVE_FUNCTION`
};

/// @brief Граф, прочитанный из файла, вместе с мнениями и параметрами, если они там есть.
struct GraphFile {
    od::graph::Graph graph{0};
    std::vector<int> opinions; ///< Мнения из секции OPINIONS или пусто, если секции нет.
    bool paramsLoaded = false; ///< Была ли в файле секция PARAMS.
    SimulationParams params;   ///< Параметры из PARAMS, если `paramsLoaded`.
};

/// @brief Пишет функцию вероятности рёбер в виде `имя число_параметров параметры…`.
void writeFunction(std::ostream& out, const od::model::ProbabilityFunction& function);

/**
 * @brief Читает функцию вероятности рёбер в виде `имя число_параметров параметры…`.
 * @param in Поток, из которого читается функция.
 * @param source Имя файла для сообщений об ошибках.
 * @throws std::runtime_error Если функция не читается, неизвестна или некорректна.
 */
od::model::ProbabilityFunction readFunction(std::istream& in, const std::string& source);

/// @brief Пишет функцию изменения мнений в виде `имя число_параметров параметры…`.
void writeOpinionFunction(std::ostream& out, const od::model::OpinionFunction& function);

/// @brief Читает функцию изменения мнений в виде `имя число_параметров параметры…`.
/// @throws std::runtime_error Если функция не читается или некорректна.
od::model::OpinionFunction readOpinionFunction(std::istream& in, const std::string& source);

/// @brief Пишет все строки секции PARAMS, по одной на параметр.
void writeParamsLines(std::ostream& out, const SimulationParams& params);

/**
 * @brief Сохраняет граф и, если `opinions` не пуст, мнения вершин в секции OPINIONS.
 * @throws std::runtime_error Если файл не открывается или запись не удалась.
 */
void saveGraph(const od::graph::Graph& graph, const std::vector<int>& opinions, const std::string& filename);

/// @brief То же, что `saveGraph`, и дописывает секцию PARAMS.
void saveGraphWithParams(const od::graph::Graph& graph, const std::vector<int>& opinions,
                         const SimulationParams& params, const std::string& filename);

/**
 * @brief Читает граф, секцию OPINIONS и секцию PARAMS, если они есть.
 * @throws std::runtime_error Если файл не открывается или содержит ошибку.
 */
GraphFile loadGraphFile(const std::string& filename);

} // namespace od::io
