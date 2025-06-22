#include "Node.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>

Node::Node(const nlohmann::json& t)
{
    name_ = t["identifier"].get<std::string>();
}

std::string Node::toString() const
{
    return name_;
}

std::string Node::getName() const
{
    return toString();
}

bool Node::operator==(const Node& rhs) const
{
    return name_ == rhs.name_;
}
