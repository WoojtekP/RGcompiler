#include "StlContainerBase.hpp"

StlContainerBase::StlContainerBase(std::string containerTypeName, const std::vector<std::string> &variables)
: containerTypeName_(containerTypeName), prefix_("std::")
{
    generateTypeAndOrder(variables);
}

std::string StlContainerBase::getType() const
{
    return keyType_;
}

std::string StlContainerBase::getContainerDeclaration() const
{
    return prefix_ + containerTypeName_ + "<" + keyType_ + ">";
}

std::string StlContainerBase::getSetMethodDeclaration(int node) const
{
    return "insert" + getFunctionInput(node);
}

std::string StlContainerBase::getIsSetMethodDeclaration(int node) const
{
    return "count" + getFunctionInput(node);
}

std::string StlContainerBase::getAdditionalData() const
{
    return "";
}

std::string StlContainerBase::getFunctionInput(int node) const
{
    return "({" + std::to_string(node) + accessOrder_ + "})";
}

void StlContainerBase::generateTypeAndOrder(const std::vector<std::string> &variables)
{
    keyType_ += "std::tuple<int";
    for (int i = 0; i < variables.size(); i++)
    {
        keyType_ += ",decltype(" + variables[i] + ")";
        accessOrder_ += "," + variables[i];
    }

    keyType_ += ">";
}