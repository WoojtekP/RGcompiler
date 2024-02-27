#pragma once
#include <compiler/graphOperations/BaseOperator.hpp>

class PragmaUniqueOperator : public BaseOperator
{
public:
    PragmaUniqueOperator(const std::shared_ptr<Graph> &graph);
    bool areAllNodesWithPragmaUnique(const std::set<std::string> &uniqueNodes) const;
};