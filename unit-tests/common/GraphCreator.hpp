#pragma once

#include <gtest/gtest.h>

#include <compiler/graphOperations/GraphOperatorManager.hpp>
#include <graph/Graph.hpp>

namespace GraphCreator
{
class GraphFixture : public ::testing::Test
{
public:
    ValueAssigner valueAssigner_;
    std::shared_ptr<Graph> graph_;
    std::shared_ptr<GraphOperatorManager> graphOperatorManager_;
    GraphFixture()
    : graph_(std::make_shared<Graph>())
    , graphOperatorManager_(std::make_shared<GraphOperatorManager>())
    {}
};

struct NodeData
{
    std::string nodeName_;
    std::string bindingVarName_;
    std::string bindingTypeName_;
};

std::shared_ptr<Node> createNode(const std::string& nodeName);
std::shared_ptr<Node> createNode(const NodeData& nodeData);
std::shared_ptr<IAction> createTagAction(const std::string& tag);
std::shared_ptr<IAction> createAssignmentAction(const std::string& leftSide, const std::string& rightSide);
std::shared_ptr<IAction> createReachabilityAction(const std::string& leftSide, const std::string& rightSide);
void addEdge(
    std::shared_ptr<Graph>& graph,
    const std::string& fromNodeName,
    const std::string& toNodeName,
    const std::shared_ptr<IAction>& action);
void addEdge(
    std::shared_ptr<Graph>& graph,
    const NodeData& fromNode,
    const NodeData& toNode,
    const std::shared_ptr<IAction>& action);

}  // namespace GraphCreator
