#include "od/model/DynamicsModel.hpp"

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string>
#include <utility>

namespace od::model {

namespace {

constexpr int NOT_LISTED = INT_MAX;

double uniform(std::mt19937& rng) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
}

int chooseProportional(const std::vector<OpinionChance>& chances, int currentOpinion, double randomValue) {
    double cumulative = 0.0;
    bool anyPositive = false;
    for (const OpinionChance& chance : chances) {
        anyPositive = anyPositive || chance.probability > 0.0;
        cumulative += chance.probability;
        if (randomValue < cumulative) {
            return chance.opinion;
        }
    }
    return anyPositive ? chances.back().opinion : currentOpinion;
}

std::vector<int> makePopularityRank(const std::vector<int>& popularity, int numOpinions) {
    std::vector<int> rank(static_cast<size_t>(numOpinions), NOT_LISTED);
    for (size_t i = 0; i < popularity.size(); ++i) {
        rank[static_cast<size_t>(popularity[i])] = static_cast<int>(i);
    }
    return rank;
}

} // namespace

std::vector<OpinionCount> countOpinions(const std::vector<int>& opinions, int numOpinions) {
    std::vector<int> counts(static_cast<size_t>(numOpinions), 0);
    for (int opinion : opinions) {
        ++counts[static_cast<size_t>(opinion)];
    }

    std::vector<OpinionCount> result;
    for (int k = 0; k < numOpinions; ++k) {
        if (counts[static_cast<size_t>(k)] > 0) {
            result.push_back(OpinionCount{k, counts[static_cast<size_t>(k)]});
        }
    }
    std::stable_sort(result.begin(), result.end(),
                     [](const OpinionCount& a, const OpinionCount& b) { return a.count > b.count; });
    return result;
}

OpinionUpdater::OpinionUpdater(OpinionRule rule, const ChoiceSettings& choice, bool useWeights)
    : rule_(std::move(rule)), choiceRule_(choice.rule), useWeights_(useWeights) {
    std::string error = validateOpinionRule(rule_);
    if (error.empty()) {
        error = validatePopularity(choice.popularity, numOpinions());
    }
    if (!error.empty()) {
        throw std::invalid_argument("OpinionUpdater: " + error);
    }

    const size_t size = static_cast<size_t>(numOpinions());
    popularityRank_ = makePopularityRank(choice.popularity, numOpinions());
    weightByOpinion_.assign(size, 0.0);
    seen_.assign(size, 0);
}

const std::vector<OpinionShare>& OpinionUpdater::neighborShares(const Graph& graph, int vertex,
                                                                const std::vector<int>& opinions) {
    double total = 0.0;
    for (const auto& [neighbor, weight] : graph.getNeighbors(vertex)) {
        const double contribution = useWeights_ ? weight : 1.0;
        const size_t opinion = static_cast<size_t>(opinions[static_cast<size_t>(neighbor)]);
        total += contribution;
        if (!seen_[opinion]) {
            seen_[opinion] = 1;
            touched_.push_back(static_cast<int>(opinion));
        }
        weightByOpinion_[opinion] += contribution;
    }

    shares_.clear();
    std::sort(touched_.begin(), touched_.end());
    for (int opinion : touched_) {
        const size_t index = static_cast<size_t>(opinion);
        if (total > 0.0 && weightByOpinion_[index] > 0.0) {
            shares_.push_back(OpinionShare{opinion, weightByOpinion_[index] / total});
        }
        weightByOpinion_[index] = 0.0;
        seen_[index] = 0;
    }
    touched_.clear();
    return shares_;
}

int OpinionUpdater::breakTie(std::mt19937& rng) {
    int best = tied_.front();
    for (int opinion : tied_) {
        if (popularityRank_[static_cast<size_t>(opinion)] < popularityRank_[static_cast<size_t>(best)]) {
            best = opinion;
        }
    }
    if (popularityRank_[static_cast<size_t>(best)] != NOT_LISTED) {
        return best;
    }
    const size_t index = static_cast<size_t>(uniform(rng) * static_cast<double>(tied_.size()));
    return tied_[std::min(index, tied_.size() - 1)];
}

int OpinionUpdater::chooseMax(const std::vector<OpinionChance>& chances, int current, std::mt19937& rng) {
    double best = 0.0;
    for (const OpinionChance& chance : chances) {
        best = std::max(best, chance.probability);
    }
    if (best <= 0.0) {
        return current;
    }

    tied_.clear();
    for (const OpinionChance& chance : chances) {
        if (chance.probability == best) {
            tied_.push_back(chance.opinion);
        }
    }
    const int candidate = tied_.size() == 1 ? tied_.front() : breakTie(rng);
    if (candidate == current) {
        return current;
    }
    return uniform(rng) < best ? candidate : current;
}

std::vector<int> OpinionUpdater::step(const Graph& graph, const std::vector<int>& current, std::mt19937& rng) {
    std::vector<int> next = current;

    for (int v = 0; v < graph.getNumVertices(); ++v) {
        if (graph.isStubborn(v)) {
            continue;
        }
        const std::vector<OpinionShare>& shares = neighborShares(graph, v, current);
        if (shares.empty()) {
            continue;
        }
        const std::vector<OpinionChance> chances = adoptionProbabilities(rule_, shares);
        const int opinion = current[static_cast<size_t>(v)];
        next[static_cast<size_t>(v)] = choiceRule_ == ChoiceRule::Max
            ? chooseMax(chances, opinion, rng)
            : chooseProportional(chances, opinion, uniform(rng));
    }
    return next;
}

} // namespace od::model
