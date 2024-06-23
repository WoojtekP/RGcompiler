#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

struct SimpleApplySwitchTreeNode
{
    std::unordered_map<std::string, std::shared_ptr<SimpleApplySwitchTreeNode>> children_;
    std::vector<int> listOfEdges_;
    void insert(const std::vector<std::string>& tags, const std::vector<int>& edges, int currTagPos)
    {
        if (currTagPos == tags.size())
        {
            assert(listOfEdges_.empty());
            listOfEdges_ = edges;
            return;
        }
        const auto& tag = tags[currTagPos];
        if (!children_.count(tag))
        {
            children_[tag] = std::make_shared<SimpleApplySwitchTreeNode>();
        }
        children_[tag]->insert(tags, edges, currTagPos + 1);
    }
    bool empty() const { return children_.empty() && listOfEdges_.empty(); }
    void insert(const std::vector<std::string>& tags, const std::vector<int>& edges) { insert(tags, edges, 0); }
};

class PragmaSimpleApplyOperator : public BaseOperator
{
    using NodeName = std::string;
    using EdgeId = int;
    using TagId = int;
    std::map<NodeName, std::shared_ptr<SimpleApplySwitchTreeNode>> mapOfSimpleApplySwitchTreeNodeFromNode_;
    std::map<NodeName, std::vector<EdgeId>> mapOfListOfEdgesToPlayerChangeFromNode_;

    struct ParsedSingleSimpleApplyData
    {
        std::string nodeName_;
        std::vector<std::string> tagNames_;
        std::vector<std::string> nodePathToTagOrPlayerChange_;
        bool hasTag() const { return !tagNames_.empty(); };
        ParsedSingleSimpleApplyData(
            const std::string& nodeName,
            const std::vector<std::string>& nodePathToTagOrPlayerChange,
            const std::vector<std::string>& tagNames = {})
        : nodeName_(nodeName)
        , nodePathToTagOrPlayerChange_(nodePathToTagOrPlayerChange)
        , tagNames_(tagNames)
        {}
    };

    std::vector<std::string> convertTagsToFullTags(
        const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData) const;
    void updateStateForData(const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData);

    ValueAssigner* valueAssigner_ = nullptr;
    std::set<std::string> simpeApplyNodeNames_;
    // Main node means that it node without simpleApply pragma have edge to this node
    std::set<std::string> mainNodeNames_;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(ValueAssigner* valueAssigner, const Parser& parser, int gameFlag = 0);
    const std::shared_ptr<SimpleApplySwitchTreeNode>& getActionListToTags(const std::shared_ptr<Node>& node) const;
    const std::vector<EdgeId>& getActionListToPlayerChange(const std::shared_ptr<Node>& node) const;
    bool isSimpleApply(const std::string& nodeName) const;
    bool isMainSimpleApply(const std::string& nodeName) const;
};