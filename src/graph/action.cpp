#include <string>

#include <nlohmann/json.hpp>

#include <graph/action.hpp>

ActionBase::ActionBase(ActionType actionType) : ActionBase(actionType, false) {}

ActionBase::ActionBase(ActionType actionType, bool negated) : actionType_(actionType), negated_(negated) {}

ActionBase::~ActionBase() {}

void ActionBase::parse(const nlohmann::json& t)
{
    if (left_ == nullptr)
    {
        left_ = std::make_unique<Expression>();
    }

    left_->parse(t["lhs"]);

    if (right_ == nullptr)
    {
        right_ = std::make_unique<Expression>();
    }

    right_->parse(t["rhs"]);
}

bool ActionBase::getNegated()
{
    return negated_;
}

std::string ActionBase::getLeftSide()
{
    return left_->toString();
}

std::string ActionBase::getRightSide()
{
    return right_->toString();
}

ActionType ActionBase::getType()
{
    return actionType_;
}

ActionAssignment::ActionAssignment() : ActionBase(ActionType::Assignment) {}

std::string ActionAssignment::toString()
{
    return left_->toString() + " = " + right_->toString();
}

ActionComparison::ActionComparison(bool negated) : ActionBase(ActionType::Comparison, negated) {}

std::string ActionComparison::toString()
{
    return left_->toString() + " == " + right_->toString();
}

ActionPattern::ActionPattern() : ActionBase(ActionType::Pattern) {}

std::string ActionPattern::toString()
{
    return "";
}

ActionPatternAny::ActionPatternAny() : ActionBase(ActionType::PatternAny) {}

std::string ActionPatternAny::toString()
{
    return "any " + left_->toString() + " -> " + right_->toString();
}

ActionReachability::ActionReachability(bool negated) : ActionBase(ActionType::Reachability, negated) {}

std::string ActionReachability::toString()
{
    return (negated_ ? "!" : "?") + left_->toString() + " -> " + right_->toString();
}

void ActionSkip::parse(const nlohmann::json& t) {}

std::string ActionSkip::toString()
{
    return "";
}

std::string ActionSkip::getLeftSide()
{
    return "";
}

std::string ActionSkip::getRightSide()
{
    return "";
}

ActionType ActionSkip::getType()
{
    return ActionType::Skip;
}

bool ActionSkip::getNegated()
{
    return false;
}

Action::Action(const nlohmann::json& t)
{
    this->parse(t);
}

Action::Action()
{
    action_ = std::make_unique<ActionSkip>();
}

Action::~Action() {}

void Action::parse(const nlohmann::json& t)
{
    if (action_)
    {
        action_.reset();
    }

    if (t["kind"] == "Assignment")
    {
        action_ = std::make_unique<ActionAssignment>();
    }
    else if (t["kind"] == "Pattern")
    {
        action_ = std::make_unique<ActionPattern>();
    }
    else if (t["kind"] == "Reachability")
    {
        action_ = std::make_unique<ActionReachability>(t["negated"].get<bool>());
    }
    else if (t["kind"] == "Comparison")
    {
        action_ = std::make_unique<ActionComparison>(t["negated"].get<bool>());
    }
    else if (t["kind"] == "PatternAny")
    {
        action_ = std::make_unique<ActionPatternAny>();
    }
    else
    {
        action_ = std::make_unique<ActionSkip>();
    }

    action_->parse(t);
}

std::string Action::toString()
{
    if (action_)
    {
        return action_->toString();
    }

    return "";
}

ActionType Action::getType()
{
    if (action_)
    {
        return action_->getType();
    }

    return ActionType::Skip;
}

std::string Action::getLeftSide()
{
    if (action_)
    {
        return action_->getLeftSide();
    }

    return "";
}

std::string Action::getRightSide()
{
    if (action_)
    {
        return action_->getRightSide();
    }

    return "";
}

bool Action::getNegated()
{
    if (action_)
    {
        return action_->getNegated();
    }

    return false;
}
