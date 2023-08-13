#pragma once

#include <string>
#include <optional>

#include <nlohmann/json.hpp>


class Binding
{
    const std::string variableName_;
    const std::string iteratedType_;

public:
    Binding(const std::string& variableName, const std::string& iteratedType);
    std::string toString() const;
    const std::string& getVariableName() const;
    const std::string& getTypeName() const;
    bool operator==(const Binding &rhs) const;
};

class Node
{
    std::string name_;
    std::optional<Binding> binding_;

public:
    Node(const nlohmann::json &t);
    std::string toString() const;
    std::string getName() const;
    const std::optional<Binding>& getBinding() const;
    bool operator==(const Node &rhs) const;
};
