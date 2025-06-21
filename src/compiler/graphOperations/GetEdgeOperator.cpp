#include <common/Common.hpp>
#include <compiler/graphOperations/GetEdgeOperator.hpp>

GetEdgeOperator::ReturnType GetEdgeOperator::getEdges(
    const std::function<bool(const std::shared_ptr<IAction> &)> &filter) const
{
    std::set<std::string> nodes;
    ReturnType edges;

    for (auto &node : graph_->getOuterNodes())
    {
        for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(node->toString()))
        {
            for (const auto &action : edge->getActions())
            {
                if (filter(action))
                {
                    if (nodes.find(edge->toName()) == nodes.end())
                    {
                        edges.insert(std::make_pair(edge, iid));
                        nodes.insert(edge->toName());
                    }
                }
            }
        }
    }

    return edges;
}

GetEdgeOperator::ReturnType GetEdgeOperator::getEdgesWithActionChangePlayer() const
{
    return getEdges([](const std::shared_ptr<IAction> &action) { return common::isActionAssignmentToPlayer(action); });
}

GetEdgeOperator::ReturnType GetEdgeOperator::getEdgesWithActionChangePlayerButNotKeeper() const
{
    return getEdges(
        [](const std::shared_ptr<IAction> &action) { return common::isActionAssignmentToPlayerButNotKeeper(action); });
}

GetEdgeOperator::ReturnType GetEdgeOperator::getEdgesWithActionTag() const
{
    return getEdges([](const std::shared_ptr<IAction> &action) {
        return action->getType() == ActionType::Tag || action->getType() == ActionType::TagVariable;
    });
}

const std::vector<std::tuple<std::string, std::string, int>> &GetEdgeOperator::getEdgeNames()
{
    if (!edgeNames_.empty())
    {
        return edgeNames_;
    }
    for (auto [edge, iid] : graph_->getAllEdges())
    {
        edgeNames_.push_back({edge->fromName(), edge->toName(), iid});
    }
    return edgeNames_;
}

std::vector<std::tuple<std::string, std::string, int>> GetEdgeOperator::getUnambiguousPathFromNode(
    const std::string &name, bool checkPlayerChange) const
{
    std::string node = name;
    std::vector<std::tuple<std::string, std::string, int>> path;

    while (graph_->getOutgoingEdgesFrom(node).size() == 1)
    {
        const auto &[edge, iid] = graph_->getOutgoingEdgesFrom(node).back();
        const auto &firstAction = edge->getActions().front();
        if (firstAction->getType() == ActionType::Tag || firstAction->getType() == ActionType::TagVariable)
        {
            return path;
        }

        path.push_back(std::make_tuple(edge->fromName(), edge->toName(), iid));
        if (checkPlayerChange)
        {
            for (const auto &action : edge->getActions())
            {
                if (common::isActionAssignmentToPlayer(action))
                {
                    return path;
                }
            }
        }

        if (node == edge->toName())
        {
            break;
        }

        node = edge->toName();
    }

    return path;
}

GetEdgeOperator::GetEdgeOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}
