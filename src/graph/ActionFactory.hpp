#pragma once

#include <memory>

#include <compiler/SymbolsManager.hpp>
#include <graph/Action.hpp>
#include <graph/ExpressionFactory.hpp>
#include <parser/Parser.hpp>

class ActionFactory
{
public:
    ActionFactory(const Parser& parser, const SymbolsManager& symbolsManager);
    std::shared_ptr<IAction> createAction(const nlohmann::json& label);

private:
    std::shared_ptr<IAction> createActionAssignment(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionReachability(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionComparison(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionAssignmentAny(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionSkip();
    std::shared_ptr<IAction> createActionTag(const nlohmann::json& label);
    std::shared_ptr<IAction> createActionTagVariable(const nlohmann::json& label);
    const Parser& parser_;
    const SymbolsManager& symbolsManager_;
    ExpressionFactory expressionFactory_;
};
