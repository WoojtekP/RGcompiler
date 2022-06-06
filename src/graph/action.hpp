#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <graph/expression.hpp>


class ActionI
{
public:
    virtual ~ActionI() {}
    virtual std::string toString() = 0;
    virtual void parse(const nlohmann::json& t) = 0;
};

class ActionBase : public ActionI
{
protected:
    ExpressionI *left_;
    ExpressionI *right_;

public:
    ActionBase();
    ~ActionBase();

    void parse(const nlohmann::json& t);
};

class ActionAssignment : public ActionBase
{
public:
    std::string toString();
};
// TODO: comparasion can be negated
class ActionComparison : public ActionBase
{
public:
    std::string toString();
};
// TODO: pattern need to be implemented
class ActionPattern : public ActionBase
{
public:
    std::string toString();
};

class ActionSkip : public ActionI
{
public:
    void parse(const nlohmann::json& t);

    std::string toString();
};

class Action : public ActionI
{
    ActionI *action_ = nullptr;

public:
    ~Action();
    Action(const nlohmann::json& t);

    void parse(const nlohmann::json& t);

    std::string toString();
};
