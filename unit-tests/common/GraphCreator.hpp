#pragma once

#include <graph/Graph.hpp>

namespace GraphCreator
{
std::shared_ptr<Node> createNode(const std::string& nodeName);

std::shared_ptr<IAction> createAssignmentAction(const std::string& leftSide, const std::string& rightSide);

void addEdge(
    std::shared_ptr<Graph>& graph,
    const std::string& fromNodeName,
    const std::string& toNodeName,
    const std::shared_ptr<IAction>& action);
}  // namespace GraphCreator