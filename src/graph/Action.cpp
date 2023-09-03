#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include <graph/Action.hpp>

ActionBase::ActionBase(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: left_(expressionFactory.createExpression(label["lhs"]))
, right_(expressionFactory.createExpression(label["rhs"]))
, negated_(label.count("negated") ? label["negated"].get<bool>() : false)
{}

bool ActionBase::getNegated() const
{
    return negated_;
}

std::string ActionBase::getLeftSide() const
{
    return left_->toString();
}

std::string ActionBase::getRightSide() const
{
    return right_->toString();
}

ActionAssignment::ActionAssignment(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionAssignment::toString() const
{
    return left_->toString() + " = " + right_->toString();
}

ActionType ActionAssignment::getType() const
{
    return ActionType::Assignment;
}

ActionComparison::ActionComparison(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionComparison::toString() const
{
    return left_->toString() + (getNegated() ? "!=" : "==") + right_->toString();
}

ActionType ActionComparison::getType() const
{
    return ActionType::Comparison;
}

ActionPattern::ActionPattern(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionPattern::toString() const
{
    return "";
}

ActionType ActionPattern::getType() const
{
    return ActionType::Pattern;
}

ActionPatternAny::ActionPatternAny(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionPatternAny::toString() const
{
    return "any " + left_->toString() + " -> " + right_->toString();
}

ActionType ActionPatternAny::getType() const
{
    return ActionType::PatternAny;
}

ActionReachability::ActionReachability(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionReachability::toString() const
{
    return (negated_ ? "!" : "?") + left_->toString() + " -> " + right_->toString();
}

ActionType ActionReachability::getType() const
{
    return ActionType::Reachability;
}

std::string ActionSkip::toString() const
{
    return "";
}

std::string ActionSkip::getLeftSide() const
{
    return "";
}

std::string ActionSkip::getRightSide() const
{
    return "";
}

ActionType ActionSkip::getType() const
{
    return ActionType::Skip;
}

bool ActionSkip::getNegated() const
{
    return false;
}

ActionTag::ActionTag(const nlohmann::json& label)
{
    tag_ = label["symbol"];
}

std::string ActionTag::toString() const
{
    return tag_;
}

std::string ActionTag::getLeftSide() const
{
    return "";
}

std::string ActionTag::getRightSide() const
{
    return "";
}

ActionType ActionTag::getType() const
{
    return ActionType::Tag;
}

bool ActionTag::getNegated() const
{
    return false;
}
