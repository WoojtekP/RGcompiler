#include <string>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "Edge.hpp"

#include <graph/Node.hpp>


Edge::Edge(
    const std::shared_ptr<Node> &from,
    const std::shared_ptr<Node> &to,
    const std::vector<std::shared_ptr<IAction>> &actions)
: Edge(from, to, actions, {})
{}

Edge::Edge(
    const std::shared_ptr<Node> &from,
    const std::shared_ptr<Node> &to,
    const std::vector<std::shared_ptr<IAction>> &actions,
    const std::vector<std::shared_ptr<Node>> &innerNodes)
: from_(from), to_(to), actions_(actions.begin(), actions.end()), innerNodes_(innerNodes.begin(), innerNodes.end()) {};

Edge::~Edge() {}

bool Edge::operator==(const Edge &edge) const
{
    const auto &innerNodes = edge.getInnerNodes();

    if (fromName() != edge.fromName() || toName() != edge.toName() || innerNodes_.size() != innerNodes.size())
    {
        return false;
    }

    for (size_t i = 0; i < innerNodes_.size(); i++)
    {
        if (innerNodes_[i] != innerNodes[i])
        {
            return false;
        }
    }
    return true;
}

std::string Edge::toString() const
{
    std::string actions;

    for (const auto &action : actions_)
    {
        actions += ", " + action->toString();
    }

    return "<" + from_->toString() + ", " + to_->toString() + actions + ">";
}

std::string Edge::fromName() const
{
    if (from_)
    {
        return from_->toString();
    }

    return "";
}

std::string Edge::toName() const
{
    if (to_)
    {
        return to_->toString();
    }

    return "";
}

std::string Edge::fullName() const
{
    if (from_ && to_)
    {
        return "edge_" + fromName() + "_" + toName();
    }

    return "";
}

std::string Edge::actionToString() const
{
    std::string actions;

    for (const auto &action : actions_)
    {
        actions = action->toString() + ";\n";
    }

    return actions;
}

ActionType Edge::getActionType() const
{
    return actions_.front()->getType();
}

std::string Edge::getActionLeftSide() const
{
    return actions_.front()->getLeftSide();
}

std::string Edge::getActionRightSide() const
{
    return actions_.front()->getRightSide();
}

bool Edge::getActionNegationValue() const
{
    return actions_.front()->getNegated();
}

bool Edge::isComplementaryTo(const Edge &rhs) const
{
    return *from_ == *rhs.from_ && getActionType() == rhs.getActionType() &&
           getActionLeftSide() == rhs.getActionLeftSide() && getActionRightSide() == rhs.getActionRightSide() &&
           getActionNegationValue() != rhs.getActionNegationValue();
}

const std::vector<std::shared_ptr<IAction>> &Edge::getActions() const
{
    return actions_;
}

std::shared_ptr<Node> Edge::getLeftNode() const
{
    return from_;
}

std::shared_ptr<Node> Edge::getRightNode() const
{
    return to_;
}

const std::vector<std::shared_ptr<Node>> &Edge::getInnerNodes() const
{
    return innerNodes_;
}
