#include "PragmaSimpleApplyOperator.hpp"

#include <graph/ExpressionFactory.hpp>
#include <parser/Parser.hpp>
namespace
{
constexpr std::string_view pragmaSimpleApply = "SimpleApply";
constexpr std::string_view pragmaSimpleApplyExhaustive = "SimpleApplyExhaustive";
}  // namespace

void SimpleApplySwitchTreeNode::insert(
    const std::vector<std::string>& tags,
    std::vector<std::shared_ptr<IAction>> actions,
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

const std::pair<std::vector<std::shared_ptr<IAction>>, std::unique_ptr<Node>>&
PragmaSimpleApplyOperator::getActionListToPlayerChange(const std::shared_ptr<Node>& node) const
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
        if (tag.contains("Variable"))
        {
            const auto& tagVar = tag["Variable"];
            std::string tagVarName = tagVar["identifier"].get<std::string>();
            if (tagVar["type_"] != nullptr)
            {
                std::string tagType = tagVar["type_"]["identifier"];
                data.tagNames_.push_back("(" + tagVarName + " : " + tagType + ")");
            }
            else
            {
                data.tagNames_.push_back(tagVarName);
            }
        }
        else if (tag.contains("Symbol"))
        {
            const auto& tagName = tag["Symbol"]["symbol"].get<std::string>();
            data.tagNames_.push_back(tagName);
        }
        else
        {
            throw std::runtime_error("[PragmaSimpleApplyOperator] Unknown type of tag");
        }
    }

    for (const auto& action : item["assignments"])
    {
        data.actionsToTagOrPlayerChange_.push_back(std::make_shared<ActionAssignment>(action, expressionFactory));
    }

    updateStateForData(data);

    mainNodeNames_.insert(data.startNodeName_);

    if (isExhaustive)
        exhaustiveNodeNames_.insert(data.startNodeName_);
}

void PragmaSimpleApplyOperator::parsePragma(
    const Parser& parser, const ExpressionFactory& expressionFactory, std::string_view pragmaName)
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
        {parsedSingleSimpleApplyData.startNodeName_, std::make_pair(std::vector<std::shared_ptr<IAction>>(), nullptr)});

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
