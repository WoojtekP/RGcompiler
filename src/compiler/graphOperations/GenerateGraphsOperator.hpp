#pragma once
#include <compiler/graphOperations/BaseOperator.hpp>

class GenerateGraphsOperator : public BaseOperator
{
    std::vector<std::string> getNodesBeforeWhichPlayerChangeToKeeper() const;
    std::shared_ptr<Graph> generateGraphForPattern(
        std::string from, const std::set<int> &to, const std::set<int> &bannedEdges = std::set<int>()) const;
    std::vector<std::string> nodesToPlayerChangeOrEnd(const std::string &nodeName) const;
    void prepareOrderForSCC(
        int nodeId,
        std::vector<int> &orderOfNodes,
        const std::shared_ptr<Graph> &revGraph,
        const std::shared_ptr<Graph> &orderGraph,
        std::set<int> &visited) const;
    void assignToSCC(
        int nodeId,
        int sccId,
        const std::shared_ptr<Graph> &revGraph,
        const std::shared_ptr<Graph> &orderGraph,
        std::map<int, int> &nodeIdToSccId) const;
    bool getSccIdsOnPathToFinalNodes(
        int nodeId,
        const std::set<int> &finalNodeSccIds,
        const std::map<int, std::set<int>> &connectionsInSCCGraph,
        std::set<int> &visited,
        std::map<int, bool> &idToHavePathToFinalNode,
        std::set<int> &sccIdsOnPathToFinalNodes) const;

public:
    GenerateGraphsOperator(const std::shared_ptr<Graph> &graph);
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> forPatterns(ActionType actionType) const;
    std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>> forApplyAnyMove() const;
    std::shared_ptr<Graph> forMainGraph() const;
};