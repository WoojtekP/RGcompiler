#include <nlohmann/json.hpp>

#include <graph/Expression.hpp>
#include <parser/Parser.hpp>

ExpressionBinaryBase::ExpressionBinaryBase(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right)
: left_(std::move(left))
, right_(std::move(right))
{}

ExpressionAccess::ExpressionAccess(
    std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right, const int minValue)
: ExpressionBinaryBase(std::move(left), std::move(right))
, minValue_(minValue)
{}

std::string ExpressionAccess::toString() const
{
    if (minValue_ > 0)
    {
        return left_->toString() + "[" + right_->toString() + " - " + std::to_string(minValue_) + "]";
    }
    return left_->toString() + "[" + right_->toString() + "]";
}

ExpressionCast::ExpressionCast(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right)
: ExpressionBinaryBase(std::move(left), std::move(right))
{}

std::string ExpressionCast::toString() const
{
    // TODO: Why casting isn't working?
    // return "static_cast<" + left_->toString() + ">(" + right_->toString() + ")";
    return right_->toString();
}

ExpressionUnaryBase::ExpressionUnaryBase(const std::string& identifier)
: identifier_(identifier)
{}

std::string ExpressionUnaryBase::toString() const
{
    return identifier_;
}

ExpressionReference::ExpressionReference(const std::string& identifier)
: ExpressionUnaryBase(identifier)
{}

ExpressionTypeReference::ExpressionTypeReference(const std::string& identifier)
: ExpressionUnaryBase(identifier)
{}

ExpressionEdgeName::ExpressionEdgeName(const std::string& identifier)
: ExpressionUnaryBase(identifier)
{}
