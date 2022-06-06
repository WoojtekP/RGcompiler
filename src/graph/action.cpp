#include <string>

#include <nlohmann/json.hpp>

#include <graph/action.hpp>


ActionBase::ActionBase() : left_(new Expression), right_(new Expression)
{
}

ActionBase::~ActionBase()
{
    delete left_;
    delete right_;
}

void ActionBase::parse(const nlohmann::json& t)
{
    left_  -> parse(t["lhs"]);
    right_ -> parse(t["rhs"]);
}

std::string ActionAssignment::toString()
{
    return left_ -> toString() + " = " + right_ -> toString();
}

std::string ActionComparison::toString()
{
    return left_ -> toString() + " == " + right_ -> toString();
}

std::string ActionPattern::toString()
{
    return "";
}

void ActionSkip::parse(const nlohmann::json& t)
{
}

std::string ActionSkip::toString()
{
    return "";
}

Action::~Action()
{
    delete action_;
}

Action::Action(const nlohmann::json& t)
{
    this -> parse(t);
}

void Action::parse(const nlohmann::json& t)
{
    if (action_)
    {
        delete action_;
    }

    if (t["kind"] == "Assignment")
    {
        action_ = new ActionAssignment;
    }
    else if (t["kind"] == "Pattern")
    {
        action_ = new ActionPattern;
    }
    else if (t["kind"] == "Comparison")
    {
        action_ = new ActionComparison;
    }
    else
    {
        action_ = new ActionSkip;
    }

    action_ -> parse(t);
}

std::string Action::toString()
{
    if (action_)
    {
        return action_ -> toString();
    }

    return "";
}
