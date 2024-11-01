#pragma once
#include <iostream>
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

const std::string pragmaSimpleApply = "SimpleApply";
const std::string pragmaSimpleApplyExhaustive = "SimpleApplyExhaustive";

struct SimpleApplySwitchTreeNode
{
    std::unordered_map<std::string, std::shared_ptr<SimpleApplySwitchTreeNode>> children_;
    std::vector<int> listOfEdges_;
    bool empty() const { return children_.empty() && listOfEdges_.empty(); }
    void insert(const std::vector<std::string>& tags, const std::vector<int>& edges, int currTagPos);
    void insert(const std::vector<std::string>& tags, const std::vector<int>& edges)
    {
        std::cout << "Insert: "
                  << "\n";
        for (auto xd : tags)
        {
            std::cout << xd << ", ";
        }
        std::cout << "\n\n";
        insert(tags, edges, 0);
    }
};

class PragmaSimpleApplyOperator : public BaseOperator
{
public:
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

private:
    using NodeName = std::string;
    using EdgeId = int;
    using TagId = int;
    std::map<NodeName, std::shared_ptr<SimpleApplySwitchTreeNode>> mapOfSimpleApplySwitchTreeNodeFromNode_;
    std::map<NodeName, std::vector<EdgeId>> mapOfListOfEdgesToPlayerChangeFromNode_;

    std::optional<std::string> edgeHasTag(const std::shared_ptr<Edge>& edge) const;
    std::vector<std::string> convertTagsToFullTags(
        const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData) const;
    void updateStateForData(const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData);
    void parsePragma(const Parser& parser, const std::string& pragmaName);
    void parseItem(const nlohmann::json& item, bool isExhaustive);

    std::set<std::string> exhaustiveNodeNames_;
    std::set<std::string> simpeApplyNodeNames_;
    // Main node means that it node without simpleApply pragma have edge to this node
    std::set<std::string> mainNodeNames_;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(const Parser& parser);
    const std::shared_ptr<SimpleApplySwitchTreeNode>& getActionListToTags(const std::shared_ptr<Node>& node) const;
    const std::vector<EdgeId>& getActionListToPlayerChange(const std::shared_ptr<Node>& node) const;
    bool isSimpleApply(const std::string& nodeName) const;
    bool isExhaustive(const std::string& nodeName) const;
    bool isMainSimpleApply(const std::string& nodeName) const;
};