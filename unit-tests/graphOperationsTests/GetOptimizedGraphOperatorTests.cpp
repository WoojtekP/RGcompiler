#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>
#include <compiler/graphOperations/GraphOperatorManager.hpp>

class GraphFixture : public ::testing::Test
{
public:
    std::shared_ptr<Graph> graph_;
    std::shared_ptr<GraphOperatorManager> graphOperatorManager_;
    GraphFixture()
    : graph_(std::make_shared<Graph>())
    , graphOperatorManager_(std::make_shared<GraphOperatorManager>())
    {}
};

TEST_F(GraphFixture, TestSimplePath1)
{
    GraphCreator::addEdge(graph_, "1", "2", GraphCreator::createAssignmentAction("val", "1"));
    GraphCreator::addEdge(graph_, "2", "3", GraphCreator::createAssignmentAction("val", "2"));
    GraphCreator::addEdge(graph_, "3", "4", GraphCreator::createAssignmentAction("val", "3"));
    GraphCreator::addEdge(graph_, "3", "5", GraphCreator::createAssignmentAction("val", "4"));
    GraphCreator::addEdge(graph_, "4", "6", GraphCreator::createAssignmentAction("val", "5"));
    GraphCreator::addEdge(graph_, "5", "7", GraphCreator::createAssignmentAction("val", "6"));
    GraphCreator::addEdge(graph_, "3", "7", GraphCreator::createAssignmentAction("val", "7"));

    graph_->initialize();

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths();

    EXPECT_TRUE(graph->getEdgeOptional("1", "3", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "6", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "7", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "7", 1));
}

TEST_F(GraphFixture, TestSimplePathAssingToPlayer)
{
    GraphCreator::addEdge(graph_, "1", "2", GraphCreator::createAssignmentAction("val", "1"));
    GraphCreator::addEdge(graph_, "2", "3", GraphCreator::createAssignmentAction("player", "2"));
    GraphCreator::addEdge(graph_, "3", "4", GraphCreator::createAssignmentAction("val", "3"));
    GraphCreator::addEdge(graph_, "4", "5", GraphCreator::createAssignmentAction("player", "4"));
    GraphCreator::addEdge(graph_, "5", "6", GraphCreator::createAssignmentAction("val", "5"));
    GraphCreator::addEdge(graph_, "6", "7", GraphCreator::createAssignmentAction("player", "6"));
    GraphCreator::addEdge(graph_, "7", "8", GraphCreator::createAssignmentAction("val", "7"));

    graph_->initialize();

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths();

    EXPECT_TRUE(graph->getEdgeOptional("1", "3", 0));
    EXPECT_TRUE(graph->getEdgeOptional("3", "5", 0));
    EXPECT_TRUE(graph->getEdgeOptional("5", "7", 0));
}

TEST_F(GraphFixture, TestSimplePathWithCycles)
{
    // 1->3,3->1
    GraphCreator::addEdge(graph_, "1", "2", GraphCreator::createAssignmentAction("val", "1"));
    GraphCreator::addEdge(graph_, "2", "3", GraphCreator::createAssignmentAction("player", "2"));
    GraphCreator::addEdge(graph_, "3", "1", GraphCreator::createAssignmentAction("val", "3"));
    // 1->1
    GraphCreator::addEdge(graph_, "1", "4", GraphCreator::createAssignmentAction("val", "4"));
    GraphCreator::addEdge(graph_, "4", "5", GraphCreator::createAssignmentAction("val", "5"));
    GraphCreator::addEdge(graph_, "5", "1", GraphCreator::createAssignmentAction("val", "6"));
    // a->b,a->b,b->d
    GraphCreator::addEdge(graph_, "a", "b", GraphCreator::createAssignmentAction("val", "7"));
    GraphCreator::addEdge(graph_, "a", "b", GraphCreator::createAssignmentAction("val", "8"));
    GraphCreator::addEdge(graph_, "b", "c", GraphCreator::createAssignmentAction("val", "9"));
    GraphCreator::addEdge(graph_, "c", "d", GraphCreator::createAssignmentAction("val", "10"));
    //a1 -> a1
    GraphCreator::addEdge(graph_, "a1", "b1", GraphCreator::createAssignmentAction("val", "8"));
    GraphCreator::addEdge(graph_, "b1", "c1", GraphCreator::createAssignmentAction("val", "9"));
    GraphCreator::addEdge(graph_, "c1", "a1", GraphCreator::createAssignmentAction("val", "10"));

    graph_->initialize();

    auto graph = graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths();

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
