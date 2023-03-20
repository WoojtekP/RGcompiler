#include "SetContainer.hpp"

SetContainer::SetContainer(const std::vector<std::string> &variables) : StlContainerBase("set", variables) {}

ContainerType SetContainer::getContainerType() const
{
  return ContainerType::Set;
}
