#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

class GetVariableOperator : public BaseOperator
{
    std::set<std::string> variables_;
    void init();

public:
    GetVariableOperator(const std::shared_ptr<Graph> &graph);
    const std::set<std::string> &getVariables() const;
};