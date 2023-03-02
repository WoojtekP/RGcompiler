#pragma once

#include <memory>

#include <compiler/valueAssigner.hpp>
#include <graph/action.hpp>
#include <graph/expressionFactory.hpp>
#include <parser/parser.hpp>


class ActionFactory
{
public:
    ActionFactory(const Parser& parser, const ValueAssigner& valueAssigner);
    std::shared_ptr<IAction> createAction(const nlohmann::json& label);

private:
    std::shared_ptr<IAction> createActionAssignment(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionPattern(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionReachability(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionComparison(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionPatternAny(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionSkip();
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
    ExpressionFactory expressionFactory_;
};
