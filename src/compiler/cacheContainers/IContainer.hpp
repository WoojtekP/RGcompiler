#pragma once

#include <string>

enum class ContainerType
{
    Set,
    UnorderedSet,
    BitArray
};

class IContainer
{
public:
    virtual ~IContainer() = default;
    virtual ContainerType getContainerType() const = 0;
    virtual std::string getType() const = 0;
    virtual std::string getContainerDeclaration() const = 0;
    virtual std::string getSetMethodDeclaration(int node) const = 0;
    virtual std::string getIsSetMethodDeclaration(int node) const = 0;
    virtual std::string getAdditionalData() const = 0;
};
