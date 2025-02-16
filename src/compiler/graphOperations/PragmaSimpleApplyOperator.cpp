#include "PragmaSimpleApplyOperator.hpp"

#include <graph/ExpressionFactory.hpp>
#include <parser/Parser.hpp>


void SimpleApplySwitchTreeNode::insert(
    const std::vector<std::string>& tags,
    std::vector<std::unique_ptr<IAction>> actions,
    std::unique_ptr<Node> endNode,
    int currTagPos)
{
    if (currTagPos == tags.size())
    {
        listOfActions_ = std::move(actions);
        endNode_ = std::move(endNode);
        return;
    }
    const auto& tag = tags[currTagPos];
    if (!children_.count(tag))
    {
        children_[tag] = std::make_shared<SimpleApplySwitchTreeNode>();
    }
    children_[tag]->insert(tags, std::move(actions), std::move(endNode), currTagPos + 1);
}

const std::shared_ptr<SimpleApplySwitchTreeNode>& PragmaSimpleApplyOperator::getActionListToTags(
    const std::shared_ptr<Node>& node) const
{
    return mapOfSimpleApplySwitchTreeNodeFromNode_.at(node->getName());
}

std::pair<std::vector<std::unique_ptr<IAction>>, std::unique_ptr<Node>>&
PragmaSimpleApplyOperator::getActionListToPlayerChange(const std::shared_ptr<Node>& node)
{
    return mapOfListOfActionsToPlayerChangeFromNodeAndEndNode_.at(node->getName());
}

void PragmaSimpleApplyOperator::parseItem(
    const nlohmann::json& item, const ExpressionFactory& expressionFactory, bool isExhaustive)
{
    ParsedSingleSimpleApplyData data;
    std::unique_ptr<Node> startNode = std::make_unique<Node>(item["lhs"]);
    data.startNodeName_ = startNode->toString();
    data.endNode_ = std::make_unique<Node>(item["rhs"]);

    std::vector<std::string> tags;
    for (const auto& tag : item["tags"])
    {
        std::string tagVarName = tag["tag"].get<std::string>();
        if (tag["type"] != nullptr)
        {
            std::string tagType = tag["type"]["identifier"];
            data.tagNames_.push_back("(" + tagVarName + " : " + tagType + ")");
        }
        else
        {
            data.tagNames_.push_back(tagVarName);
        }
    }

    for (const auto& action : item["assignments"])
    {
        data.actionsToTagOrPlayerChange_.push_back(std::make_unique<ActionAssignment>(action, expressionFactory));
    }

    updateStateForData(data);

    mainNodeNames_.insert(data.startNodeName_);

    if (isExhaustive)
        exhaustiveNodeNames_.insert(data.startNodeName_);
}

void PragmaSimpleApplyOperator::parsePragma(
    const Parser& parser, const ExpressionFactory& expressionFactory, const std::string& pragmaName)
{
    for (const auto& pragma : parser.getPragmas(pragmaName))
    {
        parseItem(pragma, expressionFactory, pragmaName == pragmaSimpleApplyExhaustive);
    }
}

void PragmaSimpleApplyOperator::init(const Parser& parser, const ValueAssigner& valueAssigner)
{
    ExpressionFactory expressionFactory(parser, valueAssigner);
    parsePragma(parser, expressionFactory, pragmaSimpleApply);
    parsePragma(parser, expressionFactory, pragmaSimpleApplyExhaustive);
}

void PragmaSimpleApplyOperator::updateStateForData(ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData)
{
    // Make sure this data always have even empty value
    mapOfSimpleApplySwitchTreeNodeFromNode_.insert(
        {parsedSingleSimpleApplyData.startNodeName_, std::make_shared<SimpleApplySwitchTreeNode>()});
    mapOfListOfActionsToPlayerChangeFromNodeAndEndNode_.insert(
        {parsedSingleSimpleApplyData.startNodeName_, std::make_pair(std::vector<std::unique_ptr<IAction>>(), nullptr)});

    if (parsedSingleSimpleApplyData.hasTag())
    {
        mapOfSimpleApplySwitchTreeNodeFromNode_[parsedSingleSimpleApplyData.startNodeName_]->insert(
            parsedSingleSimpleApplyData.tagNames_,
            std::move(parsedSingleSimpleApplyData.actionsToTagOrPlayerChange_),
            std::move(parsedSingleSimpleApplyData.endNode_));
    }
    else
    {
        nodesWithAnyEmptyTagSequence_.insert(parsedSingleSimpleApplyData.startNodeName_);
        mapOfListOfActionsToPlayerChangeFromNodeAndEndNode_[parsedSingleSimpleApplyData.startNodeName_] = {
            std::move(parsedSingleSimpleApplyData.actionsToTagOrPlayerChange_),
            std::move(parsedSingleSimpleApplyData.endNode_)};
    }
}

bool PragmaSimpleApplyOperator::isExhaustive(const std::string& nodeName) const
{
    return exhaustiveNodeNames_.count(nodeName);
}

bool PragmaSimpleApplyOperator::isSimpleApply(const std::string& nodeName) const
{
    return simpeApplyNodeNames_.count(nodeName);
}

bool PragmaSimpleApplyOperator::isMainSimpleApply(const std::string& nodeName) const
{
    return mainNodeNames_.count(nodeName);
}

bool PragmaSimpleApplyOperator::hasAnyEmptyTagSequence(const std::string& nodeName) const
{
    return nodesWithAnyEmptyTagSequence_.count(nodeName);
}

PragmaSimpleApplyOperator::PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}
