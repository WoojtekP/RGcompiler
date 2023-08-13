#pragma once

#include <string>

#include <nlohmann/json.hpp>

class IExpression
{
public:
    virtual ~IExpression() = default;
    virtual std::string toString() const = 0;
};

class ExpressionBinaryBase : public IExpression
{
public:
    ExpressionBinaryBase(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right);
    ~ExpressionBinaryBase() = default;

protected:
    std::unique_ptr<IExpression> left_;
    std::unique_ptr<IExpression> right_;
};

class ExpressionAccess : public ExpressionBinaryBase
{
public:
    ExpressionAccess(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right, const int minValue = 0);
    ~ExpressionAccess() = default;
    std::string toString() const override;

private:
    const int minValue_;
};

class ExpressionCast : public ExpressionBinaryBase
{
public:
    ExpressionCast(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right);
    ~ExpressionCast() = default;
    std::string toString() const override;
};

class ExpressionUnaryBase : public IExpression
{
public:
    ExpressionUnaryBase(const std::string& identifier);
    ~ExpressionUnaryBase() = default;
    std::string toString() const override;

private:
    std::string identifier_;
};

class ExpressionReference : public ExpressionUnaryBase
{
public:
    ExpressionReference(const std::string& identifier);
    ~ExpressionReference() = default;
};

class ExpressionTypeReference : public ExpressionUnaryBase
{
public:
    ExpressionTypeReference(const std::string& identifier);
    ~ExpressionTypeReference() = default;
};

class ExpressionEdgeName : public ExpressionUnaryBase
{
public:
    ExpressionEdgeName(const std::string& identifier);
    ~ExpressionEdgeName() = default;
    // std::string toString() const override;
};
