#include <compiler/graphOperations/PragmaSimpleApplyOperator.hpp>

std::pair<std::vector<TagAndListOfEdges>, std::vector<int>> PragmaSimpleApplyOperator::getActionList(
    const std::shared_ptr<Node>& node) const
{
    assert(valueAssigner_);
    std::vector<int> edgesOnPath;
    std::vector<int> edgesOnPathToPlayerChange;
    std::set<std::string> visitedTags;
    std::set<int> visitedNodes;
    std::vector<TagAndListOfEdges> actionList;

    dfs(graph_->getNodeId(node->toString()),
        edgesOnPath,
        visitedTags,
        visitedNodes,
        actionList,
        edgesOnPathToPlayerChange);

    return {actionList, edgesOnPathToPlayerChange};
}

void PragmaSimpleApplyOperator::dfs(
    int node,
    std::vector<int>& edgesOnPath,
    std::set<std::string>& visitedTags,
    std::set<int>& visitedNodes,
    std::vector<TagAndListOfEdges>& actionList,
    std::vector<int>& edgesOnPathToPlayerChange) const
{
    if (!visitedNodes.insert(node).second)
    {
        return;
    }

    for (const auto& [edge, iid] : graph_->getOutgoingEdgesFrom(node))
    {
        edgesOnPath.push_back(graph_->getEdgeId(edge->getLeftNode()->getName(), edge->getRightNode()->getName(), iid));
        bool endSearch = false;

        std::vector<std::shared_ptr<Node>> toNodes = edge->getInnerNodes();
        toNodes.push_back(edge->getRightNode());
        auto toNodeIt = toNodes.begin();
        assert(toNodes.size() == edge->getActions().size());
        for (const auto& action : edge->getActions())
        {
            std::string tagVal;
            if (action->getType() == ActionType::Tag)
            {
                if (!(*toNodeIt)->getBinding())
                {
                    tagVal = action->toString();
                }
                else
                {
                    tagVal = (*toNodeIt)->getBinding()->toTagStringId();
                }

                if (visitedTags.insert(tagVal).second)
                {
                    actionList.push_back({tagVal, edgesOnPath});
                }
                endSearch = true;
            }
            else if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
            {
                edgesOnPathToPlayerChange = edgesOnPath;
                endSearch = true;
            }

            if (endSearch)
            {
                break;
            }
            toNodeIt++;
        }

        if (!endSearch)
        {
            dfs(graph_->getNodeId(edge->getRightNode()->getName()),
                edgesOnPath,
                visitedTags,
                visitedNodes,
                actionList,
                edgesOnPathToPlayerChange);
        }

        edgesOnPath.pop_back();
    }
}

void PragmaSimpleApplyOperator::init(ValueAssigner* valueAssigner, const Parser& parser)
{
    valueAssigner_ = valueAssigner;
    for (const auto& pragma : parser.getPragmas("SimpleApply"))
    {
        for (const auto& edge : pragma["edgeNames"])
        {
            simpeApplyNodeNames_.insert(Node(edge["parts"]).toString());
        }
    }

    for (const auto& [edge, iid] : graph_->getAllEdges())
    {
        if (!simpeApplyNodeNames_.count(edge->getLeftNode()->getName()) &&
            simpeApplyNodeNames_.count(edge->getRightNode()->getName()))
        {
            mainNodeNames_.insert(edge->getRightNode()->getName());
        }
        else if (simpeApplyNodeNames_.count(edge->getRightNode()->getName()))
        {
            for (const auto& action : edge->getActions())
            {
                if (action->getType() == ActionType::Tag ||
                    (action->getType() == ActionType::Assignment && action->getLeftSide() == "player"))
                {
                    mainNodeNames_.insert(edge->getRightNode()->getName());
                    break;
                }
            }
        }
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