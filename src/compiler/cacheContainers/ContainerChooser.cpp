#include "ContainerChooser.hpp"

namespace
{
const auto INIT_CACHE = R"(RgCache()
{
    pattern_cache.reserve(2);
    pattern_cache.resize(1);
}
)";

const auto RESET_MAIN_PART = R"(inline void reset()
{
    currentMrId = 0;
)";

const auto RESET_MAIN_CACHE_PART = R"(
    depth = 0;
    pattern_cache[0].clear();
    state_cache.clear();
)";

const auto CLEAR_CURRENT = R"(inline void clearCurrent() { pattern_cache[depth].clear(); }
)";

const auto INSERT_2 = R"(inline bool insert(const GameState& gameState, const uint nodeId)
{
    return pattern_cache[depth].insert(std::make_tuple(gameState, nodeId)).second;
})";

const auto INSERT_3 =
    R"(inline bool insert(const GameState& gameState, const move_representation& mr, const uint nodeId)
{
    return state_cache.insert(std::make_tuple(gameState, mr, nodeId)).second;
})";

const auto INC_DEPTH = R"(inline void incDepth()
{
    ++depth;
    if (depth >= pattern_cache.size())
    {
        pattern_cache.push_back({});
    }
    else
    {
        pattern_cache[depth].clear();
    }
}
)";

const auto DEC_DEPTH = R"(inline void decDepth() { --depth; }
)";

std::string getResetMethod(const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache, bool removeCache)
{
    std::string clearingCaches;
    for (const auto& [_, cache] : stateToCache)
    {
        clearingCaches += cache->getCacheName() + cache->getResetInstruction() + ";\n";
    }
    return std::string(RESET_MAIN_PART) + (removeCache ? "" : std::string(RESET_MAIN_CACHE_PART)) + clearingCaches +
           "}\n";
}
}  // namespace

ContainerChooser::ContainerChooser(const std::string& cacheName)
: cacheName_(cacheName)
{}

std::string ContainerChooser::getAdditionalData(
    const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache, bool removeCache) const
{
    return createCache(stateToCache, removeCache);
}

std::string ContainerChooser::createCache(
    const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache, bool removeCache) const
{
    std::string rgCache = "class " + cacheName_ + "{\n";
    rgCache += "public:\n";
    if (!removeCache)
    {
        rgCache += INIT_CACHE;
    }
    rgCache += getResetMethod(stateToCache, removeCache);
    if (!removeCache)
    {
        rgCache += CLEAR_CURRENT;
        rgCache += INSERT_2;
        rgCache += INSERT_3;
        rgCache += INC_DEPTH;
        rgCache += DEC_DEPTH;
        rgCache += "\n";
    }

    if (!removeCache)
    {
        rgCache += "unsigned depth = 0;\n";
        rgCache +=
            "std::unordered_set<std::tuple<GameState, move_representation, uint>, GameState::Hasher> state_cache;\n";
        rgCache += "std::vector<std::unordered_set<std::tuple<GameState, uint>, GameState::Hasher>> pattern_cache;\n";
    }
    rgCache += "uint currentMrId = 0;\n";
    for (const auto& [_, cache] : stateToCache)
    {
        rgCache += cache->getCacheType() + " " + cache->getCacheName() + ";\n";
    }
    rgCache += "};\n";
    return rgCache;
}
