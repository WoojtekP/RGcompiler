#pragma once

#include "StlContainerBase.hpp"

class SetContainer : public StlContainerBase
{
public:
    SetContainer(const std::vector<std::string> &variables);
    ContainerType getContainerType() const override;
};