#include "ExpressionFactory.hpp"

#include <graph/Expression.hpp>

ExpressionFactory::ExpressionFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
{}

std::unique_ptr<IExpression> ExpressionFactory::createExpression(const nlohmann::json& expression) const
{
    const auto& expressionKind = expression["kind"].get<std::string>();
    if (expressionKind == "Reference")
    {
        return createReferenceExpression(expression);
    }
    if (expressionKind == "TypeReference")
    {
        return createTypeReferenceExpression(expression);
    }
    if (expressionKind == "Access")
    {
        return createAccessExpression(expression);
    }
    if (expressionKind == "Cast")
    {
        return createCastExpression(expression);
    }
    if (expressionKind == "EdgeName")
    {
        return createEdgeNameExpression(expression);
    }
    throw std::runtime_error("[ExpressionFactory] Unknown type of expression " + expressionKind);
}

std::unique_ptr<IExpression> ExpressionFactory::createReferenceExpression(const nlohmann::json& expression) const
{
    const auto identifier = expression["identifier"].get<std::string>();
    return std::make_unique<ExpressionReference>(identifier);
}

std::unique_ptr<IExpression> ExpressionFactory::createTypeReferenceExpression(const nlohmann::json& expression) const
{
    const auto identifier = expression["identifier"].get<std::string>();
    return std::make_unique<ExpressionTypeReference>(identifier);
}

std::unique_ptr<IExpression> ExpressionFactory::createAccessExpression(const nlohmann::json& expression) const
{
    auto left = createExpression(expression["lhs"]);
    auto right = createExpression(expression["rhs"]);
    const auto sourceType = parser_.getSourceType(parser_.findTypeOfExpression(expression["lhs"]));
    //const auto [minValue, maxValue] = valueAssigner_.getTypeMinMaxValues(sourceType);
    return std::make_unique<ExpressionAccess>(std::move(left), std::move(right), 0);
}

std::unique_ptr<IExpression> ExpressionFactory::createCastExpression(const nlohmann::json& expression) const
{
    auto left = createExpression(expression["lhs"]);
    auto right = createExpression(expression["rhs"]);
    return std::make_unique<ExpressionCast>(std::move(left), std::move(right));
}

std::unique_ptr<IExpression> ExpressionFactory::createEdgeNameExpression(const nlohmann::json& expression) const
{
    if (const auto& node = Parser::getPartFromParts(expression["parts"], "Literal"))
    {
        return std::make_unique<ExpressionEdgeName>(node->get()["identifier"].get<std::string>());
    }
    throw std::runtime_error(
        "[ExpressionFactory] Cannot find part: \"Literal\" for " + expression["kind"].get<std::string>());
}
