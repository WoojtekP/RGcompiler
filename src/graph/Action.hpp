#pragma once

#include <string>

#include <nlohmann/json.hpp>

class ExpressionFactory;
class IExpression;
class Parser;

enum class ActionType
{
    Assignment,
    Reachability,
    Comparison,
    Skip,
    AssignmentAny,
    Tag,
    TagVariable,
};

class IAction
{
public:
    virtual ~IAction() = default;
    virtual std::string toString() const = 0;
    virtual std::string getLeftSide() const = 0;
    virtual std::string getRightSide() const = 0;
    virtual ActionType getType() const = 0;
    virtual bool getNegated() const = 0;
};

class ActionBase : public IAction
{
public:
    ActionBase(const nlohmann::json& label, const ExpressionFactory& expressionFactory);
    std::string getLeftSide() const override;
    std::string getRightSide() const override;
    bool getNegated() const override;

protected:
    std::unique_ptr<IExpression> left_;
    std::unique_ptr<IExpression> right_;
    bool negated_;
};

class ActionAssignment : public ActionBase
{
public:
    ActionAssignment(const nlohmann::json& label, const ExpressionFactory& expressionFactory);
    std::string toString() const override;
    ActionType getType() const override;
};

class ActionComparison : public ActionBase
{
public:
    ActionComparison(const nlohmann::json& label, const ExpressionFactory& expressionFactory);
    std::string toString() const override;
    ActionType getType() const override;
};

class ActionReachability : public ActionBase
{
public:
    ActionReachability(const nlohmann::json& label, const ExpressionFactory& expressionFactory);
    std::string toString() const override;
    ActionType getType() const override;
};

class ActionAssignmentAny : public ActionBase
{
public:
    ActionAssignmentAny(const nlohmann::json& label, const ExpressionFactory& expressionFactory);
    std::string toString() const override;
    ActionType getType() const override;
};

class ActionSkip : public IAction
{
public:
    ActionSkip() = default;
    std::string toString() const override;
    std::string getLeftSide() const override;
    std::string getRightSide() const override;
    ActionType getType() const override;
    bool getNegated() const override;
};

class ActionTag : public IAction
{
    std::string tag_;

public:
    ActionTag(const nlohmann::json& label);
    std::string toString() const override;
    std::string getLeftSide() const override;
    std::string getRightSide() const override;
    ActionType getType() const override;
    bool getNegated() const override;
};

class ActionTagVariable : public IAction
{
    std::string tag_;
    std::string type_;

public:
    ActionTagVariable(const nlohmann::json& label, const Parser& parser);
    std::string toString() const override;
    std::string getLeftSide() const override;
    std::string getRightSide() const override;
    ActionType getType() const override;
    bool getNegated() const override;
};
