#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <compiler/stateCache/IStateCache.hpp>

#include "BitArrayContainer.hpp"
#include "IContainer.hpp"
#include "SetContainer.hpp"
#include "UnorderedSetContainer.hpp"

using IdType = std::tuple<std::string, std::string, int>;
using VariableAndDomain = std::vector<std::pair<std::string, int>>;

class ContainerChooser
{
private:
    const std::string cacheName_;

    std::string createCache(
        const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache, bool removeCache) const;

public:
    ContainerChooser(const std::string& cacheName);
    std::string getAdditionalData(
        const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache, bool removeCache) const;
};
