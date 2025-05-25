#include "ExpressionFactory.hpp"

#include <graph/Expression.hpp>

ExpressionFactory::ExpressionFactory(const Parser& parser, const SymbolsManager& symbolsManager)
: parser_(parser)
, symbolsManager_(symbolsManager)
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
        if (expression["lhs"]["kind"] == "Reference")
        {
            const auto& mapName = expression["lhs"]["identifier"].get<std::string>();
            for (const auto& [constant, arithmeticData] : symbolsManager_.constantToArithmeticOperationMap())
            {
                if (mapName.starts_with(constant))
                {
                    return createArithmeticExpression(expression, constant, arithmeticData);
                }
            }
        }
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
    auto sourceType = parser_.getSourceType(parser_.findTypeOfExpression(expression["lhs"]));
    const auto [minValue, maxValue] = symbolsManager_.getValueAssigner().getTypeMinMaxValues(sourceType);
    return std::make_unique<ExpressionAccess>(std::move(left), std::move(right), minValue);
}

std::unique_ptr<IExpression> ExpressionFactory::createCastExpression(const nlohmann::json& expression) const
{
    auto left = createExpression(expression["lhs"]);
    auto right = createExpression(expression["rhs"]);
    return std::make_unique<ExpressionCast>(std::move(left), std::move(right));
}

std::unique_ptr<IExpression> ExpressionFactory::createEdgeNameExpression(const nlohmann::json& expression) const
{
    const auto identifier = expression["identifier"].get<std::string>();
    return std::make_unique<ExpressionEdgeName>(identifier);
}

std::unique_ptr<IExpression> ExpressionFactory::createArithmeticExpression(
    const nlohmann::json& accessExpression, const std::string& constant, const ArithmeticData arithmeticData) const
{
    const auto mapType = parser_.findTypeOfVariable(constant);
    const auto srcTypeId = parser_.getSourceType(mapType);
    const auto dstTypeId = parser_.getDestinationType(mapType)["identifier"].get<std::string>();
    const auto& valueAssigner = symbolsManager_.getValueAssigner();
    const auto [srcMinSymbol, srcMaxSymbol] = valueAssigner.getTypeMinMaxSymbols(srcTypeId);
    const auto [dstMinSymbol, dstMaxSymbol] = valueAssigner.getTypeMinMaxSymbols(dstTypeId);
    const auto rhsExpression = createExpression(accessExpression["rhs"]);
    const auto identifier = rhsExpression->toString();
    std::string borderValue, borderResult, operation;
    switch (arithmeticData.operation)
    {
        case ArithmeticOperation::Inc:
            borderValue = srcMaxSymbol;
            operation = "+";
            break;
        case ArithmeticOperation::Dec:
            borderValue = srcMinSymbol;
            operation = "-";
            break;
    }
    switch (arithmeticData.system)
    {
        case ArithmeticSystem::Overflow:
            return std::make_unique<ExpressionReference>(identifier + operation + "1");
        case ArithmeticSystem::Saturated:
            borderResult = (borderValue == srcMaxSymbol ? dstMaxSymbol : dstMinSymbol);
            break;
        case ArithmeticSystem::Modular:
            borderResult = (borderValue == srcMaxSymbol ? dstMinSymbol : dstMaxSymbol);
            break;
    }
    const auto condition = identifier + " == " + borderValue;
    return std::make_unique<ExpressionConditional>(condition, borderResult, identifier + operation + "1");
}
