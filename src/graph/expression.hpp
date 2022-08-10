#pragma once

#include <string>

#include <nlohmann/json.hpp>

enum class ExpressionType
{
    Reference,
    TypeReference,
    Access,
    Cast,
    EdgeName
};

class ExpressionI
{
public:
    virtual ~ExpressionI() = default;
    virtual std::string toString() = 0;
    virtual void parse(const nlohmann::json& t) = 0;
};

class ExpressionBinaryBase : public ExpressionI
{
protected:
    std::unique_ptr<ExpressionI> left_;
    std::unique_ptr<ExpressionI> right_;

public:
    ExpressionBinaryBase();
    ~ExpressionBinaryBase();
    void parse(const nlohmann::json& t) override;
};

class Expression : public ExpressionI
{
    std::unique_ptr<ExpressionI> expression_;

public:
    ~Expression();

    void parse(const nlohmann::json& t) override;
    std::string toString() override;
};

class ExpressionAccess : public ExpressionBinaryBase
{
public:
    std::string toString() override;
};

class ExpressionCast : public ExpressionBinaryBase
{
public:
    std::string toString() override;
};

class ExpressionUnaryBase : public ExpressionI
{
    std::string val_;

public:
    void parse(const nlohmann::json& t) override;

    std::string toString() override;
};

class ExpressionReference : public ExpressionUnaryBase
{};

class ExpressionTypeReference : public ExpressionUnaryBase
{};

class ExpressionEdgeName : public ExpressionI
{
    std::string val_;

public:
    void parse(const nlohmann::json& t) override;

    std::string toString() override;
};