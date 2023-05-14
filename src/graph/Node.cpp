#include "Node.hpp"

#include <exception>
#include <string>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>


Binding::Binding(const std::string& variableName, const std::string& iteratedType)
: variableName_(variableName), iteratedType_(iteratedType)
{}

std::string Binding::toString() const
{
    return "__bind__" + variableName_;
}

bool Binding::operator==(const Binding &rhs) const
{
    return variableName_ == rhs.variableName_ && iteratedType_ == rhs.iteratedType_;
}

Node::Node(const nlohmann::json &t)
{
    name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

    if (const auto& optBinding = Parser::getPartFromParts(t, "Binding"))
    {
        throw std::logic_error("Binds are not implemented. Use --expandGeneratorNodes to remove them when generating AST");
    }
}

std::string Node::toString() const
{
    return binding_.has_value() ? name_ + binding_->toString() : name_;
}

std::string Node::getName() const
{
    return toString();
}

const std::optional<Binding>& Node::getBinding() const
{
    return binding_;
}

bool Node::operator==(const Node &rhs) const
{
    return name_ == rhs.name_ && binding_ == rhs.binding_;
}
