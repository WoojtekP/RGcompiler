#pragma once

#include <compiler/graphOperations/BaseOperator.hpp>

class GenerateGraphsOperator : public BaseOperator
{
    std::vector<std::string> getNodesBeforeWhichPlayerChangeToKeeper() const;
    std::shared_ptr<Graph> generateGraphForPattern(
        std::string from, const std::set<int> &to, const std::set<int> &bannedEdges = std::set<int>()) const;
    std::vector<std::string> nodesToPlayerChangeOrEnd(const std::string &nodeName) const;
    bool generatePathFromNodeToNode(
        int node,
        const std::set<int> &finalNodes,
        std::map<int, int> &nodeIdToOldestParent,
        std::map<int, int> &visitTime,
        std::set<int> &nodesInPatternGraph,
        const std::set<int> &bannedEdges,
        int &timestampId) const;

public:
    GenerateGraphsOperator(const std::shared_ptr<Graph> &graph);
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> forPatterns(ActionType actionType) const;
    std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>> forApplyAnyMove() const;
    std::shared_ptr<Graph> forMainGraph() const;
};