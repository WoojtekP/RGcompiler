#pragma once
#include <compiler/graphOperations/BaseOperator.hpp>

class PragmaUniqueOperator : public BaseOperator
{
    std::optional<bool> areAllNodesWithPragmaUnique_;

public:
    PragmaUniqueOperator(const std::shared_ptr<Graph> &graph);
    void init(const std::set<std::string> &uniqueNodes);
    bool areAllNodesWithPragmaUnique(const std::set<std::string> &uniqueNodes) const;
    bool areAllNodesWithPragmaUnique() const;
};