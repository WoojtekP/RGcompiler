#include "Action.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include <graph/Expression.hpp>
#include <graph/ExpressionFactory.hpp>
#include <parser/Parser.hpp>

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

ActionAssignmentAny::ActionAssignmentAny(const nlohmann::json& label, const ExpressionFactory& expressionFactory)
: ActionBase(label, expressionFactory)
{}

std::string ActionAssignmentAny::toString() const
{
    return left_->toString() + " = " + right_->toString() + "(*)";
}

ActionType ActionAssignmentAny::getType() const
{
    return ActionType::AssignmentAny;
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
    return "$" + tag_;
}

std::string ActionTag::getLeftSide() const
{
    return tag_;
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

ActionTagVariable::ActionTagVariable(const nlohmann::json& label, const Parser& parser)
{
    tag_ = label["identifier"];
    type_ = parser.findTypeOfVariable(tag_)["identifier"].get<std::string>();
}

std::string ActionTagVariable::toString() const
{
    return "$$" + tag_;
}

std::string ActionTagVariable::getLeftSide() const
{
    return tag_;
}

std::string ActionTagVariable::getRightSide() const
{
    return type_;
}

ActionType ActionTagVariable::getType() const
{
    return ActionType::TagVariable;
}

bool ActionTagVariable::getNegated() const
{
    return false;
}
