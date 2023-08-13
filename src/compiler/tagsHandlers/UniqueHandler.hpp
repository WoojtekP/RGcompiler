#pragma once
#include <graph/Graph.hpp>

class UniqueHandler
{
    std::shared_ptr<Graph> graph_;
    const Parser &parser_;
    std::set<int> nodesOnUniquePaths_;

    void init();

public:
    UniqueHandler(const std::shared_ptr<Graph> &graph, const Parser &parser);
    bool isOnUniquePath(int node);
};
