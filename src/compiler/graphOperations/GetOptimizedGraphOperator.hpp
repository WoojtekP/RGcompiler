#pragma once

#include <map>
#include <memory>
#include <set>

#include <compiler/ValueAssigner.hpp>
#include <compiler/graphOperations/BaseOperator.hpp>

class GetOptimizedGraphOperator : public BaseOperator
{
    using NodeId = int;
    // NodeId to number of incoming edges
    std::map<NodeId, int> numberOfIncomingEdges_;
    std::shared_ptr<Graph> optimizedGraph_;

    void traverseCycle(int node, std::vector<int> &path, std::map<int, bool> &visited) const;
    void traverse(const std::shared_ptr<Edge> &edge, std::vector<int> &path, std::map<int, bool> &visited) const;
    void initializeNumberOfIncomingEdges();
    void initializeNodesUsedInReachabilityPattern();

public:
    GetOptimizedGraphOperator(const std::shared_ptr<Graph> &graph);
    std::shared_ptr<Graph> getGraphWithOptimizedPaths(const ValueAssigner &valueAssigner);
};