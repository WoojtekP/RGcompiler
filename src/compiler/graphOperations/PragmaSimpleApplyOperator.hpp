#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

using TagAndListOfEdges = std::pair<std::string, std::vector<int>>;

class PragmaSimpleApplyOperator : public BaseOperator
{
    using NodeName = std::string;
    using EdgeId = int;
    using TagId = int;
    std::map<NodeName, std::vector<TagAndListOfEdges>> mapOfListOfEdgesToTagFromNode_;
    std::map<NodeName, std::vector<EdgeId>> mapOfListOfEdgesToPlayerChangeFromNode_;

    struct ParsedSingleSimpleApplyData
    {
        std::string nodeName_;
        std::string tagName_;
        std::vector<std::string> nodePathToTagOrPlayerChange_;
        bool hasTag() const { return !tagName_.empty(); };
        ParsedSingleSimpleApplyData(
            const std::string& nodeName,
            const std::vector<std::string>& nodePathToTagOrPlayerChange,
            const std::string& tagName = "")
        : nodeName_(nodeName)
        , nodePathToTagOrPlayerChange_(nodePathToTagOrPlayerChange)
        , tagName_(tagName)
        {}
    };

    void updateStateForData(const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData);

    ValueAssigner* valueAssigner_ = nullptr;
    std::set<std::string> simpeApplyNodeNames_;
    // Main node means that it node without simpleApply pragma have edge to this node
    std::set<std::string> mainNodeNames_;

public:
    PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph);
    void init(ValueAssigner* valueAssigner, const Parser& parser, int gameFlag = 0);
    const std::vector<TagAndListOfEdges>& getActionListToTags(const std::shared_ptr<Node>& node) const;
    const std::vector<EdgeId>& getActionListToPlayerChange(const std::shared_ptr<Node>& node) const;
    bool isSimpleApply(const std::string& nodeName) const;
    bool isMainSimpleApply(const std::string& nodeName) const;
};