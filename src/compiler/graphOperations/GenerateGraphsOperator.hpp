#pragma once

#include <compiler/graphOperations/BaseOperator.hpp>

class GenerateGraphsOperator : public BaseOperator
{
    std::vector<std::string> getNodesBeforeWhichPlayerChangeToKeeper() const;
    std::shared_ptr<Graph> generateGraphForPattern(
        std::string from, std::string to, const std::set<int> &bannedEdges = std::set<int>()) const;
    std::vector<std::string> nodesToPlayerChangeOrEnd(const std::string &nodeName) const;
    bool generatePathFromNodeToNode(
        std::string node,
        std::string finalNode,
        std::vector<std::shared_ptr<Edge>> &edges,
        std::vector<bool> &visited,
        std::vector<bool> &onPathToFinalNode,
        const std::set<int> &bannedEdges) const;

public:
    GenerateGraphsOperator(const std::shared_ptr<Graph> &graph);
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> forPatterns(ActionType actionType) const;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> forApplyAnyMove() const;
};