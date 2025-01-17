#include "PragmaRepeatOperator.hpp"

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <ranges>

#include <graph/Edge.hpp>
#include <graph/Graph.hpp>

namespace
{
bool isTagOrPlayerAssignment(const std::shared_ptr<IAction>& action)
{
    return action->getType() != ActionType::Tag ||
           (action->getType() == ActionType::Assignment && action->getLeftSide() == "player");
}
}

PragmaRepeatOperator::PragmaRepeatOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}

void PragmaRepeatOperator::dfs(std::set<int>& visited, const int node)
{
    if (!visited.insert(node).second)
    {
        return;
    }

    for (const auto& [edge, _] : graph_->getOutgoingEdgesFrom(node))
    {
        if (std::ranges::all_of(edge->getActions(), isTagOrPlayerAssignment))
        {
            dfs(visited, graph_->getNodeId(edge->toName()));
        }
    }
}

std::map<std::shared_ptr<Edge>, std::set<int>> PragmaRepeatOperator::getEdgeToStatesForWhichCacheShouldBeCleared(
    const std::set<std::string>& repeatNodes,
    const std::set<std::pair<std::shared_ptr<Edge>, int>>& getEdgesWithActionTag)
{
    std::set<int> repeatNodesIds;
    for (const auto& repeatNode : repeatNodes)
    {
        const auto node = graph_->getNodeIdOptional(repeatNode);
        if (node.has_value())
        {
            repeatNodesIds.insert(*node);
        }
    }
    if (repeatNodesIds.empty())
    {
        return {};
    }
    std::map<std::shared_ptr<Edge>, std::set<int>> result;
    std::set<int> visited, repeatNodesToClear;
    for (const auto& [edge, _] : getEdgesWithActionTag)
    {
        visited.clear();
        repeatNodesToClear.clear();
        dfs(visited, graph_->getNodeId(edge->toName()));
        std::set<int> nodesToClear;
        std::ranges::set_intersection(visited, repeatNodesIds, std::inserter(nodesToClear, std::begin(nodesToClear)));
        if (!nodesToClear.empty())
        {
            result.emplace(edge, nodesToClear);
        }
    }
    return result;
}
