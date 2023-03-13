#include "containerChooser.hpp"

#include <set>

void ContainerChooser::add(const IdType &id, const VariableAndDomain &v)
{
    bool isDomainSmall = true;

    int domainSize = 1;

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

    std::unique_ptr<IContainer> container;
    // if (isDomainSmall)
    // {
    //   container = std::move(std::make_unique<BitArrayContainer>);
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

std::string ContainerChooser::getAdditionalData() const
{
    std::string result = "";
    std::set<std::string> added;

    for (const auto &[id, container] : idTypeToContainer_)
    {
        std::string data = container->getAdditionalData();

        if (added.find(data) == added.end())
        {
            added.insert(data);
            result += data;
        }
    }

    return result;
}

std::string ContainerChooser::getCustomName(const IdType &id)
{
    return "container_" + std::get<0>(id) + "_" + std::get<1>(id) + "_" + std::to_string(std::get<2>(id));
}