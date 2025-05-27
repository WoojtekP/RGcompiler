#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

class Node
{
    std::string name_;

public:
    Node(const nlohmann::json& t);
    std::string toString() const;
    std::string getName() const;
    bool operator==(const Node& rhs) const;
};
