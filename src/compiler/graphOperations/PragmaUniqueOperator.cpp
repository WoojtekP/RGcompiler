#include <compiler/graphOperations/PragmaUniqueOperator.hpp>

PragmaUniqueOperator::PragmaUniqueOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}

void PragmaUniqueOperator::init(const std::set<std::string> &uniqueNodes)
{
    for (const auto &node : graph_->getAllNodes())
    {
        if (!uniqueNodes.count(node->getName()))
        {
            areAllNodesWithPragmaUnique_ = false;
            return;
        }
    }

    areAllNodesWithPragmaUnique_ = true;
}

bool PragmaUniqueOperator::areAllNodesWithPragmaUnique(const std::set<std::string> &uniqueNodes) const
{
    for (const auto &node : graph_->getAllNodes())
    {
        if (!uniqueNodes.count(node->getName()))
        {
            return false;
        }
    }

    return true;
}

bool PragmaUniqueOperator::areAllNodesWithPragmaUnique() const
{
    assert(areAllNodesWithPragmaUnique_);

    if (areAllNodesWithPragmaUnique_)
    {
        return *areAllNodesWithPragmaUnique_;
    }

    return false;
}