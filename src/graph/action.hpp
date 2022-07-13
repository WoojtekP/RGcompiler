#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <graph/expression.hpp>

enum class ActionType
{
    Assignment,
    Reachability,
    Comparison,
    Skip,
    Pattern
};

class ActionI
{
public:
    virtual ~ActionI() = default;
    virtual std::string toString() = 0;
    virtual std::string getLeftSide() = 0;
    virtual std::string getRightSide() = 0;

    virtual ActionType getType() = 0;

    virtual bool getNegated() = 0;

    virtual void parse(const nlohmann::json& t) = 0;
};

class ActionBase : public ActionI
{
protected:
    std::unique_ptr<ExpressionI> left_;
    std::unique_ptr<ExpressionI> right_;

    ActionType actionType_;

    bool negated_;

public:
    ActionBase(ActionType actionType);
    ActionBase(ActionType actionType, bool negated);
    ~ActionBase();

    std::string getLeftSide() override;
    std::string getRightSide() override;

    ActionType getType() override;

    bool getNegated() override;

    void parse(const nlohmann::json& t) override;
};

class ActionAssignment : public ActionBase
{
public:
    ActionAssignment();
    std::string toString() override;
};
class ActionComparison : public ActionBase
{
public:
    ActionComparison(bool negated);
    std::string toString() override;
};

// TODO: pattern need to be implemented
class ActionPattern : public ActionBase
{
public:
    ActionPattern();
    std::string toString() override;
};

class ActionReachability : public ActionBase
{
public:
    ActionReachability(bool negated);
    std::string toString() override;
};

class ActionSkip : public ActionI
{
public:
    std::string toString() override;

    std::string getLeftSide() override;
    std::string getRightSide() override;

    ActionType getType() override;

    bool getNegated() override;

    void parse(const nlohmann::json& t) override;
};

class Action : public ActionI
{
    std::unique_ptr<ActionI> action_;

public:
    ~Action();
    Action(const nlohmann::json& t);

    std::string toString() override;
    std::string getLeftSide() override;
    std::string getRightSide() override;

    ActionType getType() override;

    bool getNegated() override;

    void parse(const nlohmann::json& t) override;
};
