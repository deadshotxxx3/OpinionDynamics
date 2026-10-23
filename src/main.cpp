#include "od/config/BatchArgs.hpp"
#include "od/config/BatchMode.hpp"
#include "od/config/InteractiveMode.hpp"
#include "od/model/Simulation.hpp"

#include <iostream>
#include <utility>

int main(int argc, char** argv) {
    try {
        od::config::SimulationSetup setup = od::config::hasBatchFlag(argc, argv)
            ? od::config::setupBatch(od::config::parseBatchArgs(argc, argv))
            : od::config::setupInteractive();

        od::model::runSimulation(std::move(setup.graph), setup.params, std::move(setup.initialOpinions));
    } catch (const od::config::UsageError& e) {
        std::cout << "Ошибка: " << e.what() << "\n\n";
        od::config::printBatchUsage();
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
