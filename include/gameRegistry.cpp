#include "gameRegistry.h"
#include <iostream>

namespace gameRegistry {
void runGame(const std::string &name, const GameParams &params) {

    static std::unordered_map<std::string, void (*)(const GameParams &)>
        gameToRunner = {
            {"swarm", runSwarm},
            {"tft", runTft},
            {"frostpunkTwo", runFrostpunkTwo},
            {"chess", runChess},
            {"megabonk", runMegabonk},
            {"slayTheSpire", runSlayTheSpire}};

    auto it = gameToRunner.find(name);
    if (it != gameToRunner.end()) {
        it->second(params);
    } else {
        std::cerr << "Game runner for '" << name << "' not found." << std::endl;
    }
}

} // namespace gameRegistry
