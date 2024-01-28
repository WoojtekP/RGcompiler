#pragma once
#include <compiler/graphOperations/BaseOperator.hpp>

class PragmaUniqueOperator : public BaseOperator
{
    std::set<int> nodesOnUniquePaths_;

    void uniqueDfs(int node, std::set<int> &result);
    void findUniqeNodesOnPaths(const std::set<int> &startNodes);

public:
    PragmaUniqueOperator(const std::shared_ptr<Graph> &graph);
    std::set<std::string> getNodes() const;
    void init(const Parser &parser);
    void init(const std::set<std::string> &nodes);
    bool isOnUniquePath(int node) const;
};