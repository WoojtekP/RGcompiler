#include <string>

#include <nlohmann/json.hpp>

#include <graph/expression.hpp>


ExpressionBinaryBase::ExpressionBinaryBase() : left_(new Expression), right_(new Expression)
{
}

ExpressionBinaryBase::~ExpressionBinaryBase()
{
    delete left_;
    delete right_;
}

void ExpressionBinaryBase::parse(const nlohmann::json& t)
{
    left_  -> parse(t["lhs"]);
    right_ -> parse(t["rhs"]);
}

std::string ExpressionAccess::toString()
{
    return left_ -> toString() + "[" + right_ -> toString() + "]";
}


std::string ExpressionCast::toString()
{
    return left_ -> toString() + "(" + right_ -> toString() + ")";
}


void ExpressionUnaryBase::parse(const nlohmann::json& t)
{
    val_ = t["identifier"];
}

std::string ExpressionUnaryBase::toString()
{
    return val_;
}

Expression::~Expression()
{
    delete expression_;
}

void Expression::parse(const nlohmann::json& t)
{
    if (expression_)
    {
        delete expression_;
    }

    if (t["kind"] == "Reference")
    {
        expression_ = new ExpressionReference;
    }
    else if (t["kind"] == "TypeReference")
    {
        expression_ = new ExpressionTypeReference;
    }
    else if (t["kind"] == "Access")
    {
        expression_ = new ExpressionAccess;
    }
    else if (t["kind"] == "Cast")
    {
        expression_ = new ExpressionCast;
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
