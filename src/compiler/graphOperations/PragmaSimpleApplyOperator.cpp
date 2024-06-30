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

void PragmaSimpleApplyOperator::init(const Parser& parser, int gameFlag)
{
    // for (const auto& pragma : parser.getPragmas("SimpleApply"))
    // {
    //     for (const auto& edge : pragma["edgeNames"])
    //     {
    //         simpeApplyNodeNames_.insert(Node(edge["parts"]).toString());
    //     }
    // }

    // for (const auto& [edge, iid] : graph_->getAllEdges())
    // {
    //     if (!simpeApplyNodeNames_.count(edge->getLeftNode()->getName()) &&
    //         simpeApplyNodeNames_.count(edge->getRightNode()->getName()))
    //     {
    //         mainNodeNames_.insert(edge->getRightNode()->getName());
    //     }
    //     else if (simpeApplyNodeNames_.count(edge->getRightNode()->getName()))
    //     {
    //         for (const auto& action : edge->getActions())
    //         {
    //             if (action->getType() == ActionType::Tag ||
    //                 (action->getType() == ActionType::Assignment && action->getLeftSide() == "player"))
    //             {
    //                 mainNodeNames_.insert(edge->getRightNode()->getName());
    //                 break;
    //             }
    //         }
    //     }
    // }

    //parsing should be done here, but we dont know yet how file to be parsed should looks like

    if (gameFlag == 1)
    {
        ParsedSingleSimpleApplyData d1("chooseX", {"chooseX__bind__coordX", "chooseY"}, {"coordX"});
        ParsedSingleSimpleApplyData d2("chooseY", {"chooseY__bind__coordY", "check"}, {"coordY"});
        ParsedSingleSimpleApplyData d3("check", {"set", "endmove", "checkwin"});
        updateStateForData(d1);
        updateStateForData(d2);
        updateStateForData(d3);
        mainNodeNames_.insert("chooseX");
        mainNodeNames_.insert("chooseY");
        mainNodeNames_.insert("check");
    }
    else if (gameFlag == 2)
    {
        ParsedSingleSimpleApplyData d1(
            "selectPos",
            {"selectedPos__bind__position",
             "setPos__bind__position",
             "setFinished",
             "checkOwn",
             "forward",
             "selectDirection"},
            {"position"});
        ParsedSingleSimpleApplyData d2("selectDirection", {"directionForward", "moved"}, {"F"});
        ParsedSingleSimpleApplyData d3(
            "selectDirection", {"directionLeft", "directionLeftChecked", "directionOK", "moved"}, {"L"});
        ParsedSingleSimpleApplyData d4(
            "selectDirection", {"directionRight", "directionRightChecked", "directionOK", "moved"}, {"R"});
        ParsedSingleSimpleApplyData d5("moved", {"done", "wincheck"});
        updateStateForData(d1);
        updateStateForData(d2);
        updateStateForData(d3);
        updateStateForData(d4);
        updateStateForData(d5);

        mainNodeNames_.insert("selectPos");
        mainNodeNames_.insert("selectDirection");
        mainNodeNames_.insert("moved");
    }
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