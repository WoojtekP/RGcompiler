#pragma once

#include "StlContainerBase.hpp"

class UnorderedSetContainer : public StlContainerBase
{
    static const std::string data_;

public:
    UnorderedSetContainer(const std::vector<std::string> &variables);
    std::string getContainerDeclaration() const override;
    std::string getAdditionalData() const override;
    ContainerType getContainerType() const override;
};