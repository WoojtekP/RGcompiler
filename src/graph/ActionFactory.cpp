#include "ActionFactory.hpp"

ActionFactory::ActionFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
, expressionFactory_(parser, valueAssigner)
{}

std::shared_ptr<IAction> ActionFactory::createAction(const nlohmann::json& label)
{
    const auto& labelKind = label["kind"].get<std::string>();
    if (labelKind == "Assignment")
    {
        return createActionAssignment(label);
    }
    if (labelKind == "Reachability")
    {
        return createActionReachability(label);
    }
    if (labelKind == "Comparison")
    {
        return createActionComparison(label);
    }
    if (labelKind == "AssignmentAny")
    {
        return createActionAssignmentAny(label);
    }
    if (labelKind == "Skip")
    {
        return createActionSkip();
    }
    if (labelKind == "Tag")
    {
        return createActionTag(label);
    }
    if (labelKind == "TagVariable")
    {
        return createActionTagVariable(label);
    }
    throw std::runtime_error("[ActionFactory] Unknown type of action: " + labelKind);
}

std::shared_ptr<IAction> ActionFactory::createActionAssignment(const nlohmann::json& label)
{
    return std::make_shared<ActionAssignment>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionReachability(const nlohmann::json& label)
{
    return std::make_shared<ActionReachability>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionComparison(const nlohmann::json& label)
{
    return std::make_shared<ActionComparison>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionAssignmentAny(const nlohmann::json& label)
{
    return std::make_shared<ActionAssignmentAny>(label, expressionFactory_);
}

std::shared_ptr<IAction> ActionFactory::createActionSkip()
{
    return std::make_shared<ActionSkip>();
}

std::shared_ptr<IAction> ActionFactory::createActionTag(const nlohmann::json& label)
{
    return std::make_shared<ActionTag>(label);
}

std::shared_ptr<IAction> ActionFactory::createActionTagVariable(const nlohmann::json& label)
{
    return std::make_shared<ActionTagVariable>(label, parser_);
}
