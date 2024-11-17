#include <compiler/graphOperations/PragmaSimpleApplyOperator.hpp>

void SimpleApplySwitchTreeNode::insert(
    const std::vector<std::string>& tags, const std::vector<int>& edges, int currTagPos)
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

std::optional<std::string> PragmaSimpleApplyOperator::edgeHasTag(const std::shared_ptr<Edge>& edge) const
{
    for (const auto& action : edge->getActions())
    {
        if (action->getType() == ActionType::Tag)
        {
            return action->toString();
        }
    }

    return {};
}

const std::shared_ptr<SimpleApplySwitchTreeNode>& PragmaSimpleApplyOperator::getActionListToTags(
    const std::shared_ptr<Node>& node) const
{
    return mapOfSimpleApplySwitchTreeNodeFromNode_.at(node->getName());
}

const std::vector<PragmaSimpleApplyOperator::EdgeId>& PragmaSimpleApplyOperator::getActionListToPlayerChange(
    const std::shared_ptr<Node>& node) const
{
    return mapOfListOfEdgesToPlayerChangeFromNode_.at(node->getName());
}

void PragmaSimpleApplyOperator::parseItem(const nlohmann::json& item, bool isExhaustive)
{
    std::string nodeName = item["node"]["parts"][0]["identifier"];
    if (item["node"]["parts"].size() == 2)
    {
        // TO FIX - building names with binding shouldnt be made here
        nodeName += "__bind__" + static_cast<std::string>(item["node"]["parts"][1]["identifier"]);
    }
    std::vector<std::string> tags;
    for (const auto& tag : item["tags"])
    {
        tags.push_back(tag);
    }
    std::vector<std::string> nodes;
    for (const auto& node : item["nodes"])
    {
        std::string innerNodeName = node["parts"][0]["identifier"];
        if (node["parts"].size() == 2)
        {
            // TO FIX - building names with binding shouldnt be made here
            innerNodeName += "__bind__" + static_cast<std::string>(node["parts"][1]["identifier"]);
        }
        nodes.push_back(innerNodeName);
    }

    updateStateForData({nodeName, nodes, tags});

    mainNodeNames_.insert(nodeName);

    if (isExhaustive)
        exhaustiveNodeNames_.insert(nodeName);
}

void PragmaSimpleApplyOperator::parsePragma(const Parser& parser, const std::string& pragmaName)
{
    for (const auto& pragma : parser.getPragmas(pragmaName))
    {
        parseItem(pragma, pragmaName == pragmaSimpleApplyExhaustive);
    }
}

void PragmaSimpleApplyOperator::init(const Parser& parser)
{
    parsePragma(parser, pragmaSimpleApply);
    parsePragma(parser, pragmaSimpleApplyExhaustive);
}

std::vector<std::string> PragmaSimpleApplyOperator::convertTagsToFullTags(
    const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData) const
{
    std::vector<std::string> fullTags(parsedSingleSimpleApplyData.tagNames_.size());

    int cnt = 0;
    std::string lastNodeName = parsedSingleSimpleApplyData.nodeName_;

    for (auto& currentNodeName : parsedSingleSimpleApplyData.nodePathToTagOrPlayerChange_)
    {
        assert(graph_->getEdgeIdOptional(lastNodeName, currentNodeName, 1).has_value() == false);
        if (cnt == parsedSingleSimpleApplyData.tagNames_.size())
        {
            break;
        }
        const auto& edge = graph_->getEdge(graph_->getEdgeId(lastNodeName, currentNodeName, 0));
        if (edge->getLeftNode()->getBinding() &&
            edge->getLeftNode()->getBinding()->getVariableName() == parsedSingleSimpleApplyData.tagNames_[cnt])
        {
            fullTags[cnt++] = edge->getLeftNode()->getBinding()->toTagStringId();
        }
        else if (
            edge->getRightNode()->getBinding() &&
            edge->getRightNode()->getBinding()->getVariableName() == parsedSingleSimpleApplyData.tagNames_[cnt])
        {
            fullTags[cnt++] = edge->getRightNode()->getBinding()->toTagStringId();
        }
        else if (auto edgeTag = edgeHasTag(edge))
        {
            if (*edgeTag == parsedSingleSimpleApplyData.tagNames_[cnt])
                fullTags[cnt++] = *edgeTag;
        }
        lastNodeName = currentNodeName;
    }

    return fullTags;
}

void PragmaSimpleApplyOperator::updateStateForData(const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData)
{
    std::vector<int> edges;
    if (!mapOfSimpleApplySwitchTreeNodeFromNode_.count(parsedSingleSimpleApplyData.nodeName_))
    {
        mapOfSimpleApplySwitchTreeNodeFromNode_[parsedSingleSimpleApplyData.nodeName_] =
            std::make_shared<SimpleApplySwitchTreeNode>();
    }
    if (!mapOfListOfEdgesToPlayerChangeFromNode_.count(parsedSingleSimpleApplyData.nodeName_))
    {
        mapOfListOfEdgesToPlayerChangeFromNode_[parsedSingleSimpleApplyData.nodeName_] = {};
    }

    std::string lastNodeName = parsedSingleSimpleApplyData.nodeName_;
    for (auto& currentNodeName : parsedSingleSimpleApplyData.nodePathToTagOrPlayerChange_)
    {
        assert(graph_->getEdgeIdOptional(lastNodeName, currentNodeName, 1).has_value() == false);

        edges.push_back(graph_->getEdgeId(lastNodeName, currentNodeName, 0));
        lastNodeName = currentNodeName;
    }

    if (parsedSingleSimpleApplyData.hasTag())
    {
        mapOfSimpleApplySwitchTreeNodeFromNode_[parsedSingleSimpleApplyData.nodeName_]->insert(
            convertTagsToFullTags(parsedSingleSimpleApplyData), std::move(edges));
    }
    else
    {
        mapOfListOfEdgesToPlayerChangeFromNode_[parsedSingleSimpleApplyData.nodeName_] = std::move(edges);
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

PragmaSimpleApplyOperator::PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}