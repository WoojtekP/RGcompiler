#include "Action.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include <common/ComparisonType.hpp>
#include <compiler/SymbolsManager.hpp>
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

ActionComparison::ActionComparison(
    const nlohmann::json& label,
    const ExpressionFactory& expressionFactory,
    const SymbolsManager& symbolsManager,
    const Parser& parser)
: ActionBase(label, expressionFactory)
, cmp_(getNegated() ? ComparisonType::Neq : ComparisonType::Eq)
{
    // TODO: remove comparison with 1/0
    if (label["lhs"]["kind"] != "Access" || !symbolsManager.isNan(right_->toString()))
    {
        return;
    }
    const auto lhs = label["lhs"]["lhs"]["identifier"].get<std::string>();
    const auto typeOfExpression = parser.findTypeOfExpression(label["lhs"]);
    const auto expressionTypeName = typeOfExpression["identifier"].get<std::string>();
    const auto expressionDomain = parser.getDomain(expressionTypeName);
    const auto [minSymbol, maxSymbol] = symbolsManager.getMinMaxArithmeticSymbols(expressionDomain);
    for (const auto& [constant, arithmeticData] : symbolsManager.constantToArithmeticOperationMap())
    {
        if (arithmeticData.system == ArithmeticSystem::Overflow && lhs == constant)
        {
            if (arithmeticData.operation == ArithmeticOperation::Inc && !maxSymbol.empty())
            {
                cmp_ = getNegated() ? ComparisonType::Leq : ComparisonType::Gr;
                right_ = std::make_unique<ExpressionReference>(maxSymbol);
                return;
            }
            else if (arithmeticData.operation == ArithmeticOperation::Dec && !minSymbol.empty())
            {
                cmp_ = getNegated() ? ComparisonType::Ge : ComparisonType::Less;
                right_ = std::make_unique<ExpressionReference>(minSymbol);
                return;
            }
            else if (arithmeticData.operation == ArithmeticOperation::Add || arithmeticData.operation == ArithmeticOperation::Sub)
            {
                throw std::runtime_error("[Action] ActionComparison not implemented for add/sub");
            }
        }
    }
}

std::string ActionComparison::toString() const
{
    return left_->toString() + cmpToString(cmp_) + right_->toString();
}

ActionType ActionComparison::getType() const
{
    return ActionType::Comparison;
}

ComparisonType ActionComparison::getComparisonType() const
{
    return cmp_;
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
