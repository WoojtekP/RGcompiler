#pragma once

#include <memory>
#include <vector>
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>
#include <graph/Edge.hpp>
#include <graph/Edge.hpp>


class PragmaRepeatOperator : public BaseOperator
{
private:
    void dfs(std::set<int>& visited, const int node);

public:
    PragmaRepeatOperator(const std::shared_ptr<Graph> &graph);

    std::map<std::shared_ptr<Edge>, std::set<int>> getEdgeToStatesForWhichCacheShouldBeCleared(
        const std::set<std::string>& repeatNodes,
        const std::set<std::pair<std::shared_ptr<Edge>, int>>& getEdgesWithActionTag);
};
