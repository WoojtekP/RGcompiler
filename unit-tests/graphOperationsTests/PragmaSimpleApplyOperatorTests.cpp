#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>

using namespace GraphCreator;

void addSimpleApplyDataToParsedJson(
    nlohmann::json& json, const PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData& data, bool isExhaustive = true)
{
    nlohmann::json SimpleApplys;
    for (const auto& nodeName : data.nodePathToTagOrPlayerChange_)
    {
        nlohmann::json node = {{"kind", "EdgeName"}, {"parts", {{{"identifier", nodeName}, {"kind", "Literal"}}}}};
        SimpleApplys["nodes"] += node;
    }

    SimpleApplys["tags"];
    for (const auto& tagName : data.tagNames_)
    {
        SimpleApplys["tags"] += tagName;
    }

    SimpleApplys["kind"] = isExhaustive ? "SimpleApplyExhaustive" : "SimpleApply";
    SimpleApplys["node"] = {{"kind", "EdgeName"}, {"parts", {{{"identifier", data.nodeName_}, {"kind", "Literal"}}}}};

    json["pragmas"] += SimpleApplys;
}

TEST_F(GraphFixture, TestSimpleApplyOneTag)
{
    addEdge(graph_, "1", "2", createTagAction("test"));
    addEdge(graph_, "2", "3", createAssignmentAction("val", "2"));
    addEdge(graph_, "3", "4", createAssignmentAction("val", "3"));

    graph_->initialize(valueAssigner_);
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1("1", {"2", "3", "4"}, {"test"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);
    Parser parser(parsedJson);
    simpleApplyOperator->init(parser);

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

    addEdge(
        graph_,
        NodeData({"selectPos"}),
        NodeData({"selectedPos", "position", "Position"}),
        createTagAction("position"));
    addEdge(
        graph_,
        NodeData({"selectedPos", "position", "Position"}),
        NodeData({"setPos", "position", "Position"}),
        createAssignmentAction("val", "2"));
    addEdge(
        graph_,
        NodeData({"setPos", "position", "Position"}),
        NodeData({"setFinished"}),
        createAssignmentAction("val", "3"));
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
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, d1);
    addSimpleApplyDataToParsedJson(parsedJson, d2);
    addSimpleApplyDataToParsedJson(parsedJson, d3);
    addSimpleApplyDataToParsedJson(parsedJson, d4);
    addSimpleApplyDataToParsedJson(parsedJson, d5);
    Parser parser(parsedJson);
    simpleApplyOperator->init(parser);

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

TEST_F(GraphFixture, TestSimpleApplyTicTacToe)
{
    valueAssigner_.assignValuesForSymbols({
        {{"identifier", "Coord"},
         {"kind", "TypeDeclaration"},
         {"type", {{"identifiers", nlohmann::json("[0, 1, 2]")}, {"kind", "Set"}}}},
        {{"identifier", "Player"},
         {"kind", "TypeDeclaration"},
         {"type", {{"identifiers", nlohmann::json("[w, b]")}, {"kind", "Set"}}}},
    });

    addEdge(graph_, NodeData({"move"}), NodeData({"chooseX"}), createAssignmentAction("val", "2"));
    addEdge(graph_, NodeData({"chooseX"}), NodeData({"chooseX", "coordX", "Coord"}), createTagAction("coordX"));
    addEdge(
        graph_, NodeData({"chooseX", "coordX", "Coord"}), NodeData({"chooseY"}), createAssignmentAction("val", "2"));
    addEdge(graph_, NodeData({"chooseY"}), NodeData({"chooseY", "coordY", "Coord"}), createTagAction("coordY"));
    addEdge(graph_, NodeData({"chooseY", "coordY", "Coord"}), NodeData({"check"}), createAssignmentAction("val", "2"));

    graph_->initialize(valueAssigner_);
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1(
        "chooseX", {"chooseX__bind__coordX", "chooseY", "chooseY__bind__coordY", "check"}, {"coordX", "coordY"});
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data2("move", {"chooseX"}, {});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);
    addSimpleApplyDataToParsedJson(parsedJson, data2);

    Parser parser(parsedJson);
    simpleApplyOperator->init(parser);

    std::vector<int> edgeIds;
    const std::shared_ptr<SimpleApplySwitchTreeNode>& treeFromChooseX =
        simpleApplyOperator->getActionListToTags(createNode("chooseX"));
    ASSERT_TRUE(treeFromChooseX->children_["(coordX : Coord)"]);
    ASSERT_TRUE(treeFromChooseX->children_["(coordX : Coord)"]->children_["(coordY : Coord)"]);

    edgeIds.push_back(graph_->getEdgeId("chooseX", "chooseX__bind__coordX", 0));
    edgeIds.push_back(graph_->getEdgeId("chooseX__bind__coordX", "chooseY", 0));
    edgeIds.push_back(graph_->getEdgeId("chooseY", "chooseY__bind__coordY", 0));
    edgeIds.push_back(graph_->getEdgeId("chooseY__bind__coordY", "check", 0));
    EXPECT_EQ(treeFromChooseX->children_["(coordX : Coord)"]->children_["(coordY : Coord)"]->listOfEdges_, edgeIds);
    auto edgesFromMoved = simpleApplyOperator->getActionListToPlayerChange(createNode("move"));
    EXPECT_EQ(edgesFromMoved.size(), 1);
}

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
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);
    addSimpleApplyDataToParsedJson(parsedJson, data2);
    Parser parser(parsedJson);
    simpleApplyOperator->init(parser);

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
