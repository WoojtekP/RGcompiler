#include <nlohmann/json.hpp>

#include <graph/expression.hpp>
#include <parser/parser.hpp>


ExpressionBinaryBase::ExpressionBinaryBase()
{
}

ExpressionBinaryBase::~ExpressionBinaryBase()
{
}

void ExpressionBinaryBase::parse(const nlohmann::json& t)
{
    if (left_ == nullptr)
    {
        left_ = std::make_unique<Expression>();
    }

    left_  -> parse(t["lhs"]);

    if (right_ == nullptr)
    {
        right_ = std::make_unique<Expression>();
    }

    right_ -> parse(t["rhs"]);
}

std::string ExpressionAccess::toString()
{
    return left_ -> toString() + "[" + right_ -> toString() + "]";
}

std::string ExpressionCast::toString()
{
    return "static_cast<" + left_ -> toString() + ">(" + right_ -> toString() + ")";
}

void ExpressionUnaryBase::parse(const nlohmann::json& t)
{
    val_ = t["identifier"];
}

void ExpressionEdgeName::parse(const nlohmann::json& t)
{
    const auto& node = Parser::getPartFromParts(t["parts"], "Literal");

    if (node)
    {
        val_ = (*node).get()["identifier"];
    }
}

std::string ExpressionEdgeName::toString()
{
    return val_;
}

std::string ExpressionUnaryBase::toString()
{
    return val_;
}

Expression::~Expression()
{
}

void Expression::parse(const nlohmann::json& t)
{
    if (expression_)
    {
        expression_.reset();
    }

    if (t["kind"] == "Reference")
    {
        expression_ = std::make_unique<ExpressionReference>();
    }
    else if (t["kind"] == "TypeReference")
    {
        expression_ = std::make_unique<ExpressionTypeReference>();
    }
    else if (t["kind"] == "Access")
    {
        expression_ = std::make_unique<ExpressionAccess>();
    }
    else if (t["kind"] == "Cast")
    {
        expression_ = std::make_unique<ExpressionCast>();
    }
    else if (t["kind"] == "EdgeName")
    {
        expression_ = std::make_unique<ExpressionEdgeName>();
    }

    if (expression_)
    {
        expression_ -> parse(t);
    }
}

std::string Expression::toString()
{
    if (expression_)
    {
        return expression_ -> toString();
    }

    return "";
}
