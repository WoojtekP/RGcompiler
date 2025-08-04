#include "ExpressionFactory.hpp"

#include <graph/Expression.hpp>

namespace
{
std::string getConstMapName(const nlohmann::json& expression)
{
    if (expression["lhs"]["kind"] == "Reference")
    {
        return expression["lhs"]["identifier"].get<std::string>();
    }
    else if (expression["lhs"]["kind"] == "Access" && expression["lhs"]["lhs"]["kind"] == "Reference")
    {
        return expression["lhs"]["lhs"]["identifier"].get<std::string>();
    }
    return "";
}
}  // namespace

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
        const auto mapName = getConstMapName(expression);
        if (!mapName.empty())
        {
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
    const auto dstType = parser_.getDestinationType(mapType);
    if (parser_.isArrayType(dstType))
    {
        const auto srcSndTypeId = parser_.getSourceType(dstType);
        const auto dstTypeId = parser_.getDestinationType(dstType)["identifier"].get<std::string>();
        if (srcTypeId != srcSndTypeId)
        {
            throw std::runtime_error("[ExpressionFactory] Arithmetic expression with different types not implemented");
        }
        return createBinaryArithmeticExpression(accessExpression, srcTypeId, dstTypeId, arithmeticData);
    }
    else
    {
        const auto dstTypeId = dstType["identifier"].get<std::string>();
        return createUnaryArithmeticExpression(accessExpression, srcTypeId, dstTypeId, arithmeticData);
    }
}

std::unique_ptr<IExpression> ExpressionFactory::createUnaryArithmeticExpression(
    const nlohmann::json& accessExpression,
    const std::string& srcTypeId,
    const std::string& dstTypeId,
    const ArithmeticData arithmeticData) const
{
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

std::unique_ptr<IExpression> ExpressionFactory::createBinaryArithmeticExpression(
    const nlohmann::json& accessExpression,
    const std::string& srcTypeId,
    const std::string& dstTypeId,
    const ArithmeticData arithmeticData) const
{
    const auto& valueAssigner = symbolsManager_.getValueAssigner();
    const auto [srcMinSymbol, srcMaxSymbol] = valueAssigner.getTypeMinMaxSymbols(srcTypeId);
    const auto [dstMinSymbol, dstMaxSymbol] = valueAssigner.getTypeMinMaxSymbols(dstTypeId);
    const auto lhsExpression = createExpression(accessExpression["lhs"]["rhs"]);
    const auto rhsExpression = createExpression(accessExpression["rhs"]);
    const auto lhsId = lhsExpression->toString();
    const auto rhsId = rhsExpression->toString();
    const auto srcDomainSizeStr = std::to_string(valueAssigner.getTypeRange(srcTypeId));
    std::string borderValue, condition, expression, oppositeOperator;
    switch (arithmeticData.operation)
    {
        case ArithmeticOperation::Add:
            expression = lhsId + "+" + rhsId;
            borderValue = dstMaxSymbol;
            condition = expression + ">" + borderValue;
            oppositeOperator = "-";
            break;
        case ArithmeticOperation::Sub:
            expression = lhsId + "-" + rhsId;
            borderValue = dstMinSymbol;
            condition = expression + "<" + borderValue;
            oppositeOperator = "+";
            break;
        case ArithmeticOperation::Gr:
            expression = lhsId + ">" + rhsId;
            break;
        case ArithmeticOperation::Less:
            expression = lhsId + "<" + rhsId;
            break;
        case ArithmeticOperation::Ge:
            expression = lhsId + ">=" + rhsId;
            break;
        case ArithmeticOperation::Leq:
            expression = lhsId + "<=" + rhsId;
            break;
        default:
            throw std::runtime_error("[ExpressionFactory] Unsupported binary arithmetic operation");
    }
    switch (arithmeticData.system)
    {
        case ArithmeticSystem::Overflow:
        case ArithmeticSystem::Comparison:
            return std::make_unique<ExpressionReference>("(" + expression + ")");
        case ArithmeticSystem::Saturated:
        {
            return std::make_unique<ExpressionConditional>(condition, borderValue, expression);
        }
        case ArithmeticSystem::Modular:
        {
            return std::make_unique<ExpressionConditional>(condition, borderValue + oppositeOperator + srcDomainSizeStr, expression);
        }
    }
        throw std::runtime_error("[ExpressionFactory] Unknown arithmetic system");
}
