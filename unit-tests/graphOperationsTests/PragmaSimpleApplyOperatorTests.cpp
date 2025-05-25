#include <gtest/gtest.h>

#include <common/GraphCreator.hpp>
#include <parser/Parser.hpp>

using namespace GraphCreator;

namespace
{
struct NodeId
{
    std::string leftNodeName;
    std::string rightNodeName;
    int iid;
};

std::optional<std::string> getTagVar(const std::string& tag)
{
    assert(tag.size());
    std::string tagTmp = tag.substr(1, tag.size());
    auto pos = tagTmp.find(":");

    if (pos != std::string::npos)
    {
        return tagTmp.substr(0, pos - 1);
    }
    return {};
}

std::optional<std::string> getTagType(const std::string& tag)
{
    assert(tag.size());
    std::string tagTmp = tag.substr(1, tag.size());
    auto pos = tagTmp.find(":");

    if (pos != std::string::npos)
    {
        std::string res = tagTmp.substr(pos + 2);
        res.pop_back();
        return res;
    }
    return {};
}

nlohmann::json getTag(const std::string& tag)
{
    nlohmann::json res;
    if (auto type = getTagType(tag))
    {
        res = {
            {"Variable",
             {{"identifier", *getTagVar(tag)},
              {"type_", {{"identifier", *getTagType(tag)}, {"kind", "TypeReference"}}}}}};
    }
    else
    {
        res = {{"Symbol", {{"symbol", tag}}}};
    }
    return res;
}

void addSimpleApplyDataToParsedJson(
    nlohmann::json& json, const PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData& data, bool isExhaustive = true)
{
    nlohmann::json SimpleApply;
    for (const auto& action : data.actionsToTagOrPlayerChange_)
    {
        nlohmann::json actionJson = {
            {"lhs",
             {{"kind", "Cast"},
              {"lhs", {{"kind", "TypeReference"}, {"identifier", ""}}},
              {"rhs", {{"kind", "Reference"}, {"identifier", action->getLeftSide()}}}}},
            {"rhs",
             {{"kind", "Cast"},
              {"lhs", {{"kind", "TypeReference"}, {"identifier", ""}}},
              {"rhs", {{"kind", "Reference"}, {"identifier", action->getRightSide()}}}}}};
        SimpleApply["assignments"] += actionJson;
    }

    SimpleApply["tags"];
    for (const std::string& tagName : data.tagNames_)
    {
        SimpleApply["tags"] += getTag(tagName);
    }

    SimpleApply["kind"] = isExhaustive ? "SimpleApplyExhaustive" : "SimpleApply";
    SimpleApply["lhs"] = {{"identifier", data.startNodeName_}, {"kind", "EdgeName"}};
    SimpleApply["rhs"] = {{"identifier", data.endNode_->getName()}, {"kind", "EdgeName"}};
    json["pragmas"] += SimpleApply;
}

void checkExpectations(
    const std::shared_ptr<PragmaSimpleApplyOperator>& simpleApplyOperator,
    const std::string& nodeName,
    const std::vector<std::string>& tags,
    const std::vector<std::string> expectedActions)
{
    std::shared_ptr<SimpleApplySwitchTreeNode> tree = simpleApplyOperator->getActionListToTags(createNode(nodeName));

    for (int cnt = 0; cnt < tags.size(); ++cnt)
    {
        EXPECT_TRUE(tree->children_.count(tags[cnt]));
        tree = tree->children_[tags[cnt]];
    }

    EXPECT_EQ(tree->listOfActions_.size(), expectedActions.size());
    for (int cnt = 0; cnt < expectedActions.size(); ++cnt)
    {
        const auto& action = tree->listOfActions_[cnt];
        EXPECT_EQ(action->toString(), expectedActions[cnt]);
    }
}

}  // namespace

TEST_F(GraphFixture, TestSimpleApplyOneTag)
{
    graph_->initialize(valueAssigner_);

    std::vector<std::unique_ptr<IAction>> actions;
    actions.push_back(createUniqueAssignmentAction("val1", "1"));
    actions.push_back(createUniqueAssignmentAction("val2", "2"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1(
        "1", createUniqueNode("4"), std::move(actions), {"test"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);
    Parser parser(parsedJson);
    ValueAssigner valueAssigner;
    simpleApplyOperator->init(parser, valueAssigner);

    checkExpectations(simpleApplyOperator, "1", {"test"}, {"val1 = 1", "val2 = 2"});
}

TEST_F(GraphFixture, TestSimpleApplyBreakthrough)
{
    graph_->initialize(valueAssigner_);

    std::vector<std::unique_ptr<IAction>> actions;

    actions.push_back(createUniqueAssignmentAction("player", "currentPlayer"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1(
        "begin", createUniqueNode("selectPos"), std::move(actions), {});

    actions.push_back(createUniqueAssignmentAction("player", "keeper"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data2(
        "checkOwn", createUniqueNode("forwardDirSet"), std::move(actions), {"F"});

    actions.push_back(createUniqueAssignmentAction("player", "keeper"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data3(
        "checkOwn", createUniqueNode("leftDirSet"), std::move(actions), {"L"});

    actions.push_back(createUniqueAssignmentAction("player", "keeper"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data4(
        "checkOwn", createUniqueNode("rightDirSet"), std::move(actions), {"R"});

    actions.push_back(createUniqueAssignmentAction("pos", "pos_1"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data5(
        "selectPos", createUniqueNode("checkOwn"), std::move(actions), {"(pos_1 : Position)"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);
    addSimpleApplyDataToParsedJson(parsedJson, data2);
    addSimpleApplyDataToParsedJson(parsedJson, data3);
    addSimpleApplyDataToParsedJson(parsedJson, data4);
    addSimpleApplyDataToParsedJson(parsedJson, data5);
    Parser parser(parsedJson);
    ValueAssigner valueAssigner;
    simpleApplyOperator->init(parser, valueAssigner);

    const auto& beginActionMap = simpleApplyOperator->getActionListToPlayerChange(createUniqueNode("begin"));
    const auto& beginToSelectPosActions = beginActionMap.first;
    EXPECT_EQ(beginActionMap.second->getName(), "selectPos");
    EXPECT_EQ(beginToSelectPosActions.size(), 1);
    EXPECT_EQ(beginToSelectPosActions.front()->toString(), "player = currentPlayer");

    checkExpectations(simpleApplyOperator, "checkOwn", {"F"}, {"player = keeper"});
    checkExpectations(simpleApplyOperator, "checkOwn", {"R"}, {"player = keeper"});
    checkExpectations(simpleApplyOperator, "checkOwn", {"L"}, {"player = keeper"});
    checkExpectations(simpleApplyOperator, "selectPos", {"(pos_1 : Position)"}, {"pos = pos_1"});
}

TEST_F(GraphFixture, TestSimpleApplyTicTacToe)
{
    graph_->initialize(valueAssigner_);

    std::vector<std::unique_ptr<IAction>> actions;

    actions.push_back(createUniqueAssignmentAction("posX", "posX_2"));
    actions.push_back(createUniqueAssignmentAction("posY", "posY_1"));
    actions.push_back(createUniqueAssignmentAction("board[posX][posY]", "playerTurn"));
    actions.push_back(createUniqueAssignmentAction("player", "keeper"));
    PragmaSimpleApplyOperator::ParsedSingleSimpleApplyData data1(
        "chooseX", createUniqueNode("checkwin"), std::move(actions), {"(posX_2 : Coord)", "(posX_1 : Coord)"});

    auto simpleApplyOperator = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(graph_);
    nlohmann::json parsedJson = nlohmann::json::parse(R"({"types": {}, "variables": {}, "constants": {}})");
    addSimpleApplyDataToParsedJson(parsedJson, data1);

    Parser parser(parsedJson);
    ValueAssigner valueAssigner;
    simpleApplyOperator->init(parser, valueAssigner);

    checkExpectations(
        simpleApplyOperator,
        "chooseX",
        {"(posX_2 : Coord)", "(posX_1 : Coord)"},
        {"posX = posX_2", "posY = posY_1", "board[posX][posY] = playerTurn", "player = keeper"});
}
