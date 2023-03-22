#pragma once

#include <memory>

#include <compiler/ValueAssigner.hpp>
#include <graph/Expression.hpp>
#include <parser/Parser.hpp>

class ExpressionFactory
{
public:
    ExpressionFactory(const Parser& parser, const ValueAssigner& valueAssigner);
    std::unique_ptr<IExpression> createExpression(const nlohmann::json& label) const;

private:
    std::unique_ptr<IExpression> createReferenceExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createTypeReferenceExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createAccessExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createCastExpression(const nlohmann::json& expression) const;
    std::unique_ptr<IExpression> createEdgeNameExpression(const nlohmann::json& expression) const;

    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
};
