#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>

using namespace GraphCreator;

TEST_F(GraphFixture, TestSimpleApplyOneTag)
{
    addEdge(graph_, "1", "2", createTagAction("test"));
    addEdge(graph_, "2", "3", createAssignmentAction("val", "2"));
    addEdge(graph_, "3", "4", createAssignmentAction("val", "3"));

    graph_->initialize(valueAssigner_);
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1("1", {"2", "3", "4"}, {"test"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    simpleApplyOperator->init({data1});

    const std::shared_ptr<SimpleApplySwitchTreeNode>& treeFrom1 =
        simpleApplyOperator->getActionListToTags(createNode("1"));

    EXPECT_TRUE(treeFrom1->children_.count("test"));
    std::vector<int> edgeIds;
    edgeIds.push_back(graph_->getEdgeId("1", "2", 0));
    edgeIds.push_back(graph_->getEdgeId("2", "3", 0));
    edgeIds.push_back(graph_->getEdgeId("3", "4", 0));
    EXPECT_EQ(treeFrom1->children_["test"]->listOfEdges_, edgeIds);
}

TEST_F(GraphFixture, TestSimpleApplyBreakthrough)
{
    valueAssigner_.assignValuesForSymbols({
        {{"identifier", "Position"},
         {"kind", "TypeDeclaration"},
         {"type", {{"identifiers", nlohmann::json("[null, v00]")}, {"kind", "Set"}}}},
        {{"identifier", "Player"},
         {"kind", "TypeDeclaration"},
         {"type", {{"identifiers", nlohmann::json("[w, b]")}, {"kind", "Set"}}}},
    });

    addEdge(graph_, NodeData({"selectPos"}), NodeData({"selectedPos", "Position"}), createTagAction("position"));
    addEdge(
        graph_,
        NodeData({"selectedPos", "Position"}),
        NodeData({"setPos", "Position"}),
        createAssignmentAction("val", "2"));
    addEdge(graph_, NodeData({"setPos", "Position"}), NodeData({"setFinished"}), createAssignmentAction("val", "3"));
    addEdge(graph_, "setFinished", "checkOwn", createAssignmentAction("val", "3"));
    addEdge(graph_, "checkOwn", "forward", createAssignmentAction("val", "3"));
    addEdge(graph_, "forward", "selectDirection", createAssignmentAction("val", "3"));
    addEdge(graph_, "selectDirection", "directionForward", createTagAction("F"));
    addEdge(graph_, "selectDirection", "directionLeft", createTagAction("L"));
    addEdge(graph_, "selectDirection", "directionRight", createTagAction("R"));
    addEdge(graph_, "directionForward", "moved", createAssignmentAction("val", "3"));
    addEdge(graph_, "directionLeft", "directionLeftChecked", createAssignmentAction("val", "3"));
    addEdge(graph_, "directionLeftChecked", "directionOK", createAssignmentAction("val", "3"));
    addEdge(graph_, "directionRight", "directionRightChecked", createAssignmentAction("val", "3"));
    addEdge(graph_, "directionRightChecked", "directionOK", createAssignmentAction("val", "3"));
    addEdge(graph_, "directionOK", "moved", createAssignmentAction("val", "3"));
    addEdge(graph_, "moved", "done", createAssignmentAction("val", "3"));
    addEdge(graph_, "done", "wincheck", createAssignmentAction("val", "3"));
    graph_->initialize(valueAssigner_);

    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData d1(
        "selectPos",
        {"selectedPos__bind__position",
         "setPos__bind__position",
         "setFinished",
         "checkOwn",
         "forward",
         "selectDirection"},
        {"position"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData d2("selectDirection", {"directionForward", "moved"}, {"F"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData d3(
        "selectDirection", {"directionLeft", "directionLeftChecked", "directionOK", "moved"}, {"L"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData d4(
        "selectDirection", {"directionRight", "directionRightChecked", "directionOK", "moved"}, {"R"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData d5("moved", {"done", "wincheck"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    simpleApplyOperator->init({d1, d2, d3, d4, d5});

    std::shared_ptr<SimpleApplySwitchTreeNode> treeFromSelectPos =
        simpleApplyOperator->getActionListToTags(createNode("selectPos"));
    std::vector<int> edgeIds;
    ASSERT_TRUE(treeFromSelectPos->children_.count("(position : Position)"));
    edgeIds.push_back(graph_->getEdgeId("selectPos", "selectedPos__bind__position", 0));
    edgeIds.push_back(graph_->getEdgeId("selectedPos__bind__position", "setPos__bind__position", 0));
    edgeIds.push_back(graph_->getEdgeId("setPos__bind__position", "setFinished", 0));
    edgeIds.push_back(graph_->getEdgeId("setFinished", "checkOwn", 0));
    edgeIds.push_back(graph_->getEdgeId("checkOwn", "forward", 0));
    edgeIds.push_back(graph_->getEdgeId("forward", "selectDirection", 0));
    EXPECT_EQ(treeFromSelectPos->children_["(position : Position)"]->listOfEdges_, edgeIds);

    std::shared_ptr<SimpleApplySwitchTreeNode> treeFromSelectDirection =
        simpleApplyOperator->getActionListToTags(createNode("selectDirection"));

    edgeIds.clear();
    ASSERT_TRUE(treeFromSelectDirection->children_.count("F"));
    edgeIds.push_back(graph_->getEdgeId("selectDirection", "directionForward", 0));
    edgeIds.push_back(graph_->getEdgeId("directionForward", "moved", 0));
    EXPECT_EQ(treeFromSelectDirection->children_["F"]->listOfEdges_, edgeIds);

    edgeIds.clear();
    ASSERT_TRUE(treeFromSelectDirection->children_.count("L"));
    edgeIds.push_back(graph_->getEdgeId("selectDirection", "directionLeft", 0));
    edgeIds.push_back(graph_->getEdgeId("directionLeft", "directionLeftChecked", 0));
    edgeIds.push_back(graph_->getEdgeId("directionLeftChecked", "directionOK", 0));
    edgeIds.push_back(graph_->getEdgeId("directionOK", "moved", 0));
    EXPECT_EQ(treeFromSelectDirection->children_["L"]->listOfEdges_, edgeIds);

    auto edgesFromMoved = simpleApplyOperator->getActionListToPlayerChange(createNode("moved"));
    EXPECT_EQ(edgesFromMoved.size(), 2);
}

// TODO: add this test
// TEST_F(GraphFixture, TestSimpleApplyTicTacToe)
// {
//     addEdge(graph_, "1", "2", createTagAction("test"));
//     addEdge(graph_, "2", "3", createAssignmentAction("val", "2"));
//     addEdge(graph_, "3", "4", createAssignmentAction("val", "3"));

//     graph_->initialize(valueAssigner_);
//     PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1("1", {"2", "3", "4"}, {"test"});

//     auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
//     simpleApplyOperator->init({data1});

//     const std::shared_ptr<SimpleApplySwitchTreeNode>& treeFromSelectPos =
//         simpleApplyOperator->getActionListToTags(createNode("selectPos"));

//     std::vector<int> edgeIds;
//     EXPECT_TRUE(treeFromSelectPos->children_.count("selectPos"));
//     edgeIds.push_back(graph_->getEdgeId("selectPos", "selectedPos", 0));
//     edgeIds.push_back(graph_->getEdgeId("selectedPos", "setPos", 0));
//     edgeIds.push_back(graph_->getEdgeId("setPos", "setFinished", 0));
//     edgeIds.push_back(graph_->getEdgeId("setFinished", "checkOwn", 0));
//     edgeIds.push_back(graph_->getEdgeId("checkOwn", "forward", 0));
//     edgeIds.push_back(graph_->getEdgeId("forward", "selectDirection", 0));
//     EXPECT_EQ(treeFromSelectPos->children_["selectPos"]->listOfEdges_, edgeIds);
// }

TEST_F(GraphFixture, TestSimpleMultiTag)
{
    addEdge(graph_, "1", "2", createTagAction("test1a"));
    addEdge(graph_, "1", "5", createTagAction("test2a"));
    addEdge(graph_, "2", "3", createAssignmentAction("val", "2"));
    addEdge(graph_, "3", "4", createTagAction("test1b"));
    addEdge(graph_, "5", "6", createAssignmentAction("val", "2"));
    addEdge(graph_, "6", "7", createTagAction("test2b"));

    graph_->initialize(valueAssigner_);
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1("1", {"2", "3", "4"}, {"test1a", "test1b"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data2("1", {"5", "6", "7"}, {"test2a", "test2b"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    simpleApplyOperator->init({data1, data2});

    const std::shared_ptr<SimpleApplySwitchTreeNode>& treeFrom1 =
        simpleApplyOperator->getActionListToTags(createNode("1"));

    ASSERT_TRUE(treeFrom1->children_.count("test1a"));
    ASSERT_TRUE(treeFrom1->children_.count("test2a"));
    ASSERT_TRUE(treeFrom1->children_["test1a"]->children_.count("test1b"));
    ASSERT_TRUE(treeFrom1->children_["test2a"]->children_.count("test2b"));

    std::vector<int> edgeIds;
    edgeIds.push_back(graph_->getEdgeId("1", "2", 0));
    edgeIds.push_back(graph_->getEdgeId("2", "3", 0));
    edgeIds.push_back(graph_->getEdgeId("3", "4", 0));
    EXPECT_EQ(treeFrom1->children_["test1a"]->children_["test1b"]->listOfEdges_, edgeIds);
}
