#include <compiler/graphOperations/PragmaUniqueOperator.hpp>

PragmaUniqueOperator::PragmaUniqueOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}

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