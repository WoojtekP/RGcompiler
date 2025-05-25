#pragma once

#include <set>
#include <string>
#include <string_view>

#include <compiler/SymbolsManager.hpp>
#include <compiler/graphOperations/BaseOperator.hpp>

struct TagWrapper
{
private:
    std::string getTagKey(const std::string& tag) const;

public:
    TagWrapper(const std::string& tag);
    bool operator<(const TagWrapper& tagWrapper) const { return key_ < tagWrapper.key_; }

    std::string tag_;
    std::string key_;
};

struct SimpleApplySwitchTreeNode
{
    std::map<TagWrapper, std::shared_ptr<SimpleApplySwitchTreeNode>> children_;
    std::vector<std::shared_ptr<IAction>> listOfActions_;
    std::unique_ptr<Node> endNode_;
    bool empty() const { return children_.empty() && listOfActions_.empty(); }
    void insert(
        const std::vector<std::string>& tags,
        std::vector<std::shared_ptr<IAction>> actions,
        std::unique_ptr<Node> endNode,
        int currTagPos);
    void insert(
        const std::vector<std::string>& tags,
        std::vector<std::shared_ptr<IAction>> actions,
        std::unique_ptr<Node> endNode)
    {
        insert(tags, std::move(actions), std::move(endNode), 0);
    }
};

class PragmaSimpleApplyOperator : public BaseOperator
{
public:
    struct ParsedSingleSimpleApplyData
    {
        std::string startNodeName_;
        std::unique_ptr<Node> endNode_;
        std::vector<std::string> tagNames_;
        std::vector<std::shared_ptr<IAction>> actionsToTagOrPlayerChange_;
        bool hasTag() const { return !tagNames_.empty(); };
        ParsedSingleSimpleApplyData() = default;
        ParsedSingleSimpleApplyData(
            const std::string& startNodeName,
            std::unique_ptr<Node> endNode,
            std::vector<std::shared_ptr<IAction>> actionsToTagOrPlayerChange,
            const std::vector<std::string>& tagNames = {})
        : startNodeName_(startNodeName)
        , endNode_(std::move(endNode))
        , actionsToTagOrPlayerChange_(std::move(actionsToTagOrPlayerChange))
        , tagNames_(tagNames)
        {}
    };

private:
    using NodeName = std::string;
    std::map<NodeName, std::shared_ptr<SimpleApplySwitchTreeNode>> mapOfSimpleApplySwitchTreeNodeFromNode_;
    std::map<NodeName, std::pair<std::vector<std::shared_ptr<IAction>>, std::unique_ptr<Node>>>
        mapOfListOfActionsToPlayerChangeFromNodeAndEndNode_;

    void updateStateForData(ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData);
    void parsePragma(const Parser& parser, const ExpressionFactory& expressionFactory, std::string_view pragmaName);
    void parseItem(const nlohmann::json& item, const ExpressionFactory& expressionFactory, bool isExhaustive);

    std::set<std::string> exhaustiveNodeNames_;
    std::set<std::string> simpeApplyNodeNames_;
    // Main node means that it node without simpleApply pragma have edge to this node
    std::set<std::string> mainNodeNames_;
    std::set<std::string> nodesWithAnyEmptyTagSequence_;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(const Parser& parser, const SymbolsManager& symbolsManager);
    const std::shared_ptr<SimpleApplySwitchTreeNode>& getActionListToTags(const std::shared_ptr<Node>& node) const;
    const std::pair<std::vector<std::shared_ptr<IAction>>, std::unique_ptr<Node>>& getActionListToPlayerChange(
        const std::shared_ptr<Node>& node) const;
    bool isSimpleApply(const std::string& nodeName) const;
    bool isExhaustive(const std::string& nodeName) const;
    bool isMainSimpleApply(const std::string& nodeName) const;
    bool hasAnyEmptyTagSequence(const std::string& nodeName) const;
};
