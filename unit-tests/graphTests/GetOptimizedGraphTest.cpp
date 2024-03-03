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
