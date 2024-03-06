#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>

using namespace GraphCreator;

TEST_F(GraphFixture, TestSimplePath1)
{
    addEdge(graph_, "1", "2", createAssignmentAction("val", "1"));
    addEdge(graph_, "2", "3", createAssignmentAction("val", "2"));
    addEdge(graph_, "3", "4", createAssignmentAction("val", "3"));
    addEdge(graph_, "3", "5", createAssignmentAction("val", "4"));
    addEdge(graph_, "4", "6", createAssignmentAction("val", "5"));
    addEdge(graph_, "5", "7", createAssignmentAction("val", "6"));
    addEdge(graph_, "3", "7", createAssignmentAction("val", "7"));

    graph_->initialize(valueAssigner_);

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths(
        valueAssigner_);

    EXPECT_TRUE(graph->getEdgeOptional("1", "3", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "6", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "7", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "7", 1));
}

TEST_F(GraphFixture, TestSimplePathAssingToPlayer)
{
    addEdge(graph_, "1", "2", createAssignmentAction("val", "1"));
    addEdge(graph_, "2", "3", createAssignmentAction("player", "2"));
    addEdge(graph_, "3", "4", createAssignmentAction("val", "3"));
    addEdge(graph_, "4", "5", createAssignmentAction("player", "4"));
    addEdge(graph_, "5", "6", createAssignmentAction("val", "5"));
    addEdge(graph_, "6", "7", createAssignmentAction("player", "6"));
    addEdge(graph_, "7", "8", createAssignmentAction("val", "7"));

    graph_->initialize(valueAssigner_);

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths(
        valueAssigner_);

    EXPECT_TRUE(graph->getEdgeOptional("1", "3", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "5", 0));
    EXPECT_TRUE(graph->getEdgeOptional("5", "7", 0));
}

TEST_F(GraphFixture, TestSimplePathWithCycles)
{
    // 1->3,3->1
    addEdge(graph_, "1", "2", createAssignmentAction("val", "1"));
    addEdge(graph_, "2", "3", createAssignmentAction("player", "2"));
    addEdge(graph_, "3", "1", createAssignmentAction("val", "3"));
    // 1->1
    addEdge(graph_, "1", "4", createAssignmentAction("val", "4"));
    addEdge(graph_, "4", "5", createAssignmentAction("val", "5"));
    addEdge(graph_, "5", "1", createAssignmentAction("val", "6"));
    // a->b,a->b,b->d
    addEdge(graph_, "a", "b", createAssignmentAction("val", "7"));
    addEdge(graph_, "a", "b", createAssignmentAction("val", "8"));
    addEdge(graph_, "b", "c", createAssignmentAction("val", "9"));
    addEdge(graph_, "c", "d", createAssignmentAction("val", "10"));
    //a1 -> a1
    addEdge(graph_, "a1", "b1", createAssignmentAction("val", "8"));
    addEdge(graph_, "b1", "c1", createAssignmentAction("val", "9"));
    addEdge(graph_, "c1", "a1", createAssignmentAction("val", "10"));

    graph_->initialize(valueAssigner_);

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths(
        valueAssigner_);

    EXPECT_TRUE(graph->getEdgeOptional("1", "1", 0));
    EXPECT_TRUE(graph->getEdgeOptional("1", "3", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "1", 0));
    EXPECT_TRUE(graph->getEdgeOptional("a", "b", 0));
    EXPECT_TRUE(graph->getEdgeOptional("a", "b", 0));
    EXPECT_TRUE(graph->getEdgeOptional("b", "d", 0));
    EXPECT_TRUE(
        graph->getEdgeOptional("a1", "a1", 0) || graph->getEdgeOptional("c1", "c1", 0) ||
        graph->getEdgeOptional("b1", "b1", 0));
}
