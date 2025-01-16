#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "BitArrayContainer.hpp"
#include "IContainer.hpp"
#include "SetContainer.hpp"
#include "UnorderedSetContainer.hpp"

#include <compiler/stateCache/IStateCache.hpp>

using IdType = std::tuple<std::string, std::string, int>;
using VariableAndDomain = std::vector<std::pair<std::string, int>>;

class ContainerChooser
{
private:
    const int smallDomainMaxiumSize_;
    std::map<IdType, std::unique_ptr<IContainer>> idTypeToContainer_;
    std::map<IdType, std::string> idTypeToCustomDeclaration_;
    const std::string cacheName_;

public:
    ContainerChooser(const std::string &cacheName);
    void add(const IdType &id, const VariableAndDomain &v, int nodeNumber);
    std::string getType(const IdType &id) const;
    std::string getContainerDeclaration(const IdType &id) const;
    std::string getSetMethodDeclaration(const IdType &id, int node) const;
    std::string getIsSetMethodDeclaration(const IdType &id, int node) const;
    std::string getAdditionalData(
        const std::string& gameStateAndMoveAndNodeIdHasherBody,
        const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache) const;
    std::string getCustomName(const IdType &id) const;
    std::string createCache(const std::map<std::string, std::shared_ptr<IStateCache>>& stateToCache) const;
    bool isInCache(const IdType &id) const;
    std::string getFromCache(const IdType &id) const;
    const std::map<IdType, std::string> &getIdTypeToCustomDeclaration() const;
};
