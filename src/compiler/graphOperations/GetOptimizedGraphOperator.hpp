#pragma once

#include <compiler/graphOperations/BaseOperator.hpp>

class GetOptimizedGraphOperator : public BaseOperator
{
    // NodeId to number of incoming edges
    std::map<int, int> numberOfIncomingEdges_;
    std::shared_ptr<Graph> optimizedGraph_;

    void traverseCycle(int node, std::vector<int> &path, std::map<int, bool> &visited) const;
    void traverse(int node, std::vector<int> &path, std::map<int, bool> &visited) const;
    void initializeNumberOfIncomingEdges();

public:
    GetOptimizedGraphOperator(const std::shared_ptr<Graph> &graph);
    std::shared_ptr<Graph> getGraphWithOptimizedPaths();
};