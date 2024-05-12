#include <iostream>

#include <compiler/graphOperations/PragmaSimpleApplyOperator.hpp>

const std::vector<TagAndListOfEdges>& PragmaSimpleApplyOperator::getActionListToTags(
    const std::shared_ptr<Node>& node) const
{
    return mapOfListOfEdgesToTagFromNode_.at(node->getName());
}

const std::vector<PragmaSimpleApplyOperator::EdgeId>& PragmaSimpleApplyOperator::getActionListToPlayerChange(
    const std::shared_ptr<Node>& node) const
{
    return mapOfListOfEdgesToPlayerChangeFromNode_.at(node->getName());
}

void PragmaSimpleApplyOperator::init(ValueAssigner* valueAssigner, const Parser& parser, int gameFlag)
{
    valueAssigner_ = valueAssigner;
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
        ParsedSingleSimpleApplyData d1("chooseX", {"chooseX__bind__coordX", "chooseY"}, "coordX");
        ParsedSingleSimpleApplyData d2("chooseY", {"chooseY__bind__coordY", "check"}, "coordY");
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
            "position");
        ParsedSingleSimpleApplyData d2("selectDirection", {"directionForward", "moved"}, "F");
        ParsedSingleSimpleApplyData d3(
            "selectDirection", {"directionLeft", "directionLeftChecked", "directionOK", "moved"}, "L");
        ParsedSingleSimpleApplyData d4(
            "selectDirection", {"directionRight", "directionRightChecked", "directionOK", "moved"}, "R");
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

void PragmaSimpleApplyOperator::updateStateForData(const ParsedSingleSimpleApplyData& parsedSingleSimpleApplyData)
{
    std::vector<int> edges;
    std::string lastNodeName = parsedSingleSimpleApplyData.nodeName_;
    std::string fullTag = parsedSingleSimpleApplyData.tagName_;
    if (!mapOfListOfEdgesToTagFromNode_.count(parsedSingleSimpleApplyData.nodeName_))
    {
        mapOfListOfEdgesToTagFromNode_[parsedSingleSimpleApplyData.nodeName_] = {};
    }
    if (!mapOfListOfEdgesToPlayerChangeFromNode_.count(parsedSingleSimpleApplyData.nodeName_))
    {
        mapOfListOfEdgesToPlayerChangeFromNode_[parsedSingleSimpleApplyData.nodeName_] = {};
    }

    for (auto& currentNodeName : parsedSingleSimpleApplyData.nodePathToTagOrPlayerChange_)
    {
        assert(graph_->getEdgeIdOptional(lastNodeName, currentNodeName, 1).has_value() == false);
        edges.push_back(graph_->getEdgeId(lastNodeName, currentNodeName, 0));
        auto edge = graph_->getEdge(edges.back());
        if (edge->getLeftNode()->getBinding() &&
            edge->getLeftNode()->getBinding()->getVariableName() == parsedSingleSimpleApplyData.tagName_)
        {
            fullTag = edge->getLeftNode()->getBinding()->toTagStringId();
        }
        if (edge->getRightNode()->getBinding() &&
            edge->getRightNode()->getBinding()->getVariableName() == parsedSingleSimpleApplyData.tagName_)
        {
            fullTag = edge->getRightNode()->getBinding()->toTagStringId();
        }
        lastNodeName = currentNodeName;
    }
    if (parsedSingleSimpleApplyData.hasTag())
    {
        mapOfListOfEdgesToTagFromNode_[parsedSingleSimpleApplyData.nodeName_].push_back({fullTag, std::move(edges)});
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