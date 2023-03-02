#include "actionFactory.hpp"


ActionFactory::ActionFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
, expressionFactory_(parser, valueAssigner)
{
}

std::shared_ptr<IAction> ActionFactory::createAction(const nlohmann::json& label)
{
    const auto& labelKind = label["kind"].get<std::string>();
    if (labelKind == "Assignment")
    {
        return createActionAssignment(label);
    }
    if (labelKind == "Pattern")
    {
        return createActionPattern(label);
    }
    if (labelKind == "Reachability")
    {
        return createActionReachability(label);
    }
    if (labelKind == "Comparison")
    {
        return createActionComparison(label);
    }
    if (labelKind == "PatternAny")
    {
        return createActionPatternAny(label);
    }
    if (labelKind == "Skip")
    {
        return createActionSkip();
    }
    throw std::runtime_error("[ActionFactory] Unknown type of action: " + labelKind);
}

std::shared_ptr<IAction> ActionFactory::createActionAssignment(const nlohmann::json& label)
{
    return std::make_shared<ActionAssignment>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionPattern(const nlohmann::json& label)
{
    return std::make_shared<ActionPattern>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionReachability(const nlohmann::json& label)
{
    return std::make_shared<ActionReachability>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionComparison(const nlohmann::json& label)
{
    return std::make_shared<ActionComparison>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionPatternAny(const nlohmann::json& label)
{
    return std::make_shared<ActionPatternAny>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionSkip()
{
    return std::make_shared<ActionSkip>();
}
