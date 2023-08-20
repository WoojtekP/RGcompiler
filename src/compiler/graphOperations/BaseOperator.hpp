#pragma once
#include <graph/Graph.hpp>
class BaseOperator
{
protected:
    std::shared_ptr<Graph> graph_;

public:
    BaseOperator(const std::shared_ptr<Graph>& graph);
};