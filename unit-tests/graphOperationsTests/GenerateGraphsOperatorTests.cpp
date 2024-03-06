#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>

using namespace GraphCreator;

TEST_F(GraphFixture, GenerateGraphOperatorTest1)
{
    addEdge(graph_, "1", "2", createReachabilityAction("a", "b"));
    addEdge(graph_, "2", "a", createAssignmentAction("val", "2"));
    addEdge(graph_, "a", "4", createAssignmentAction("val", "a"));
    addEdge(graph_, "a", "5", createAssignmentAction("val", "4"));
    addEdge(graph_, "5", "6", createAssignmentAction("val", "5"));
    addEdge(graph_, "5", "7", createAssignmentAction("val", "6"));
    addEdge(graph_, "6", "8", createAssignmentAction("val", "7"));
    addEdge(graph_, "7", "8", createAssignmentAction("val", "8"));
    addEdge(graph_, "4", "9", createAssignmentAction("val", "8"));
    addEdge(graph_, "8", "b", createAssignmentAction("val", "9"));
    addEdge(graph_, "b", "9", createAssignmentAction("val", "9"));

    graph_->initialize(valueAssigner_);

    auto patternReachabilityGraphs =
        graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forPatterns(ActionType::Reachability);

    EXPECT_EQ(patternReachabilityGraphs.size(), 1);
    auto pattern = patternReachabilityGraphs.back();
    EXPECT_EQ(std::get<0>(pattern), "a");
    EXPECT_EQ(std::get<1>(pattern), "b");

    auto graph = std::get<2>(pattern);
    graph->initialize(valueAssigner_);

    EXPECT_FALSE(graph->getEdgeOptional("1", "2", 0));
    EXPECT_FALSE(graph->getEdgeOptional("2", "a", 0));
    EXPECT_FALSE(graph->getEdgeOptional("a", "4", 0));
    EXPECT_TRUE(graph->getEdgeOptional("a", "5", 0));
    EXPECT_TRUE(graph->getEdgeOptional("5", "6", 0));
    EXPECT_TRUE(graph->getEdgeOptional("5", "7", 0));
    EXPECT_TRUE(graph->getEdgeOptional("6", "8", 0));
    EXPECT_TRUE(graph->getEdgeOptional("7", "8", 0));
    EXPECT_FALSE(graph->getEdgeOptional("4", "9", 0));
    EXPECT_TRUE(graph->getEdgeOptional("8", "b", 0));
    EXPECT_FALSE(graph->getEdgeOptional("b", "9", 0));
}
