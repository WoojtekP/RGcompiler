#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

using TagAndListOfEdges = std::pair<std::string, std::vector<int>>;
using PairListOfActionsToTagAndListOfActionsToPlayer = std::pair<std::vector<TagAndListOfEdges>, std::vector<int>>;

class PragmaSimpleApplyOperator : public BaseOperator
{
    ValueAssigner* valueAssigner_ = nullptr;
    std::set<std::string> simpeApplyNodeNames_;
    // Main node means that it node without simpleApply pragma have edge to this node
    std::set<std::string> mainNodeNames_;

    void dfs(
        int node,
        std::vector<int>& edgesOnPath,
        std::set<std::string>& visitedTags,
        std::set<int>& visitedNodes,
        std::vector<TagAndListOfEdges>& actionList,
        std::vector<int>& edgesOnPathToPlayerChange) const;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(ValueAssigner* valueAssigner, const Parser& parser);
    PairListOfActionsToTagAndListOfActionsToPlayer getActionList(const std::shared_ptr<Node>& node) const;
    bool isSimpleApply(const std::string& nodeName) const;
    bool isMainSimpleApply(const std::string& nodeName) const;
};