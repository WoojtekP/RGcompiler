#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

using TagAndListOfEdges = std::pair<int, std::vector<int>>;
using PairListOfActionsToTagAndListOfActionsToPlayer = std::pair<std::vector<TagAndListOfEdges>, std::vector<int>>;

class PragmaSimpleApplyOperator : public BaseOperator
{
    ValueAssigner* valueAssigner_ = nullptr;

    void dfs(
        int node,
        std::vector<int>& edgesOnPath,
        std::set<int>& visitedTags,
        std::set<int>& visitedNodes,
        std::vector<TagAndListOfEdges>& actionList,
        std::vector<int>& edgesOnPathToPlayerChange) const;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(ValueAssigner* valueAssigner);
    PairListOfActionsToTagAndListOfActionsToPlayer getActionList(const std::shared_ptr<Node>& node) const;
};