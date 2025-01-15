#include "ContainerChooser.hpp"

namespace {
const auto INIT_CACHE = R"(RgCache()
{
    pattern_cache.reserve(2);
    pattern_cache.resize(1);
}
)";

const auto RESET = R"(inline void reset()
{
    depth = 0;
    pattern_cache[0].clear();
    state_cache.clear();
}
)";

const auto CLEAR_CURRENT = R"(inline void clearCurrent() { pattern_cache[depth].clear(); }
)";

const auto INC_DEPTH = R"(inline void incDepth()
{
    ++depth;
    if (depth >= pattern_cache.size())
    {
        pattern_cache.resize(depth + 1);
    }
    else
    {
        pattern_cache[depth].clear();
    }
}
)";

const auto DEC_DEPTH = R"(inline void decDepth() { --depth; }
)";
}

ContainerChooser::ContainerChooser(const std::string &cacheName)
: smallDomainMaxiumSize_(1000)
, cacheName_(cacheName)
{}

void ContainerChooser::add(const IdType &id, const VariableAndDomain &v, int nodeNumber)
{
    int domainSize = nodeNumber;

    bool isDomainSmall = (domainSize < smallDomainMaxiumSize_ ? true : false);

    if (isDomainSmall)
    {
        for (const auto &[variable, domain] : v)
        {
            if (domain == -1 || domain > smallDomainMaxiumSize_)
            {
                isDomainSmall = false;
                break;
            }

            domainSize *= domain;

            if (domainSize > smallDomainMaxiumSize_)
            {
                isDomainSmall = false;
                break;
            }
        }
    }

    std::unique_ptr<IContainer> container;
    // if (isDomainSmall)
    // {
    //     container = std::move(std::make_unique<BitArrayContainer>(nodeNumber, v, true, cacheName_));
    // }
    // else
    // {
    std::vector<std::string> variables;
    for (const auto &[name, domain] : v)
    {
        variables.push_back(name);
    }
    container = std::make_unique<UnorderedSetContainer>(variables);
    // }

    idTypeToContainer_.emplace(std::make_pair(id, std::move(container)));
    idTypeToCustomDeclaration_.insert(std::make_pair(id, getCustomName(id)));
}

const std::map<IdType, std::string> &ContainerChooser::getIdTypeToCustomDeclaration() const
{
    return idTypeToCustomDeclaration_;
}

std::string ContainerChooser::getType(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getType();
}

std::string ContainerChooser::getContainerDeclaration(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getContainerDeclaration();
}

std::string ContainerChooser::getSetMethodDeclaration(const IdType &id, int node) const
{
    return idTypeToContainer_.at(id)->getSetMethodDeclaration(node);
}

std::string ContainerChooser::getIsSetMethodDeclaration(const IdType &id, int node) const
{
    return idTypeToContainer_.at(id)->getIsSetMethodDeclaration(node);
}

std::string ContainerChooser::getAdditionalData(const std::string& gameStateAndMoveAndNodeIdHasherBody) const
{
    std::string result = "struct StateCacheHasher{";
    result += "size_t operator()(const std::tuple<GameState,move_representation,int>& gameState) const;\n";
    result += "};\n\n";
    result += createCache() + "\n";
    return result;
}

std::string ContainerChooser::getCustomName(const IdType &id) const
{
    return "container_" + std::get<0>(id) + "_" + std::get<1>(id) + "_" + std::to_string(std::get<2>(id));
}

std::string ContainerChooser::createCache() const
{
    std::string rgCache = "class " + cacheName_ + "{\n";
    rgCache += "public:\n";
    rgCache += INIT_CACHE;
    rgCache += RESET;
    rgCache += CLEAR_CURRENT;
    rgCache += INC_DEPTH;
    rgCache += DEC_DEPTH;
    rgCache += "\n";
    rgCache += "unsigned depth = 0;\n";
    rgCache += "std::unordered_set<std::tuple<GameState, move_representation, int>, StateCacheHasher> state_cache;\n";
    rgCache += "std::vector<std::unordered_set<std::tuple<GameState, int>, GameState::gameStateHasher>> pattern_cache;\n";
    rgCache += "};\n";
    return rgCache;
}

bool ContainerChooser::isInCache(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getContainerType() == ContainerType::BitArray;
}

std::string ContainerChooser::getFromCache(const IdType &id) const
{
    return getCustomName(id);
}
