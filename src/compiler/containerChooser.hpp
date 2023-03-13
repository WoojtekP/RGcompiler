#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "BitArrayContainer.hpp"
#include "IContainer.hpp"
#include "SetContainer.hpp"
#include "UnorderedSetContainer.hpp"

class ContainerChooser
{
private:
    using IdType = std::tuple<std::string, std::string, int>;
    using VariableAndDomain = std::vector<std::pair<std::string, int>>;

    const int smallDomainMaxiumSize_ = 1000;
    std::map<IdType, std::unique_ptr<IContainer>> idTypeToContainer_;

public:
    void add(const IdType &id, const VariableAndDomain &v);
    std::string getType(const IdType &id) const;
    std::string getContainerDeclaration(const IdType &id) const;
    std::string getSetMethodDeclaration(const IdType &id, int node) const;
    std::string getIsSetMethodDeclaration(const IdType &id, int node) const;
    std::string getAdditionalData() const;
    std::string getCustomName(const IdType &id);
};
