#pragma once

#include <memory>

#include <compiler/SymbolsManager.hpp>
#include <graph/Expression.hpp>
#include <parser/Parser.hpp>

class ExpressionFactory
{
public:
    ExpressionFactory(const Parser& parser, const SymbolsManager& symbolsManager);
    std::unique_ptr<IExpression> createExpression(const nlohmann::json& label) const;

private:
    std::unique_ptr<IExpression> createReferenceExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createTypeReferenceExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createAccessExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createCastExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createEdgeNameExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createArithmeticExpression(
        const nlohmann::json& accessExpression, const std::string& constant, const ArithmeticData arithmeticData) const;
    std::unique_ptr<IExpression> createUnaryArithmeticExpression(
        const nlohmann::json& accessExpression,
        const std::string& srcTypeId,
        const std::string& dstTypeId,
        const ArithmeticData arithmeticData) const;
    std::unique_ptr<IExpression> createBinaryArithmeticExpression(
        const nlohmann::json& accessExpression,
        const std::string& srcTypeId,
        const std::string& dstTypeId,
        const ArithmeticData arithmeticData) const;

    const Parser& parser_;
    const SymbolsManager& symbolsManager_;
};
