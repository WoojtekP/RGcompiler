#include <iostream>

#include <compiler/graphOperations/PragmaSimpleApplyOperator.hpp>
std::pair<std::vector<TagAndListOfEdges>, std::vector<int>> PragmaSimpleApplyOperator::getActionList(
    const std::shared_ptr<Node>& node) const
{
    assert(valueAssigner_);
    std::vector<int> edgesOnPath;
    std::vector<int> edgesOnPathToPlayerChange;
    std::set<int> visitedTags;
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
    std::set<int>& visitedTags,
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
        std::cout << edge->getLeftNode()->getName() << " " << edge->getRightNode()->getName() << "\n";

        for (const auto& action : edge->getActions())
        {
            if (action->getType() == ActionType::Tag)
            {
                std::cout << "1\n";
                int tagId = valueAssigner_->getBaseValueForTag(action->toString());
                if (visitedTags.insert(tagId).second)
                {
                    std::cout << "12\n";
                    std::cout << edge->getRightNode()->getName() << "\n";

                    actionList.push_back({tagId, edgesOnPath});
                }
                endSearch = true;
            }
            else if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
            {
                std::cout << "2\n";

                edgesOnPathToPlayerChange = edgesOnPath;
                endSearch = true;
            }

            if (endSearch)
            {
                std::cout << "3\n";

                break;
            }
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

void PragmaSimpleApplyOperator::init(ValueAssigner* valueAssigner)
{
    valueAssigner_ = valueAssigner;
}

PragmaSimpleApplyOperator::PragmaSimpleApplyOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)

{}