#include "Node.hpp"

#include <exception>
#include <string>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>

Binding::Binding(const std::string& variableName, const std::string& iteratedType)
: variableName_(variableName)
, iteratedType_(iteratedType)
{}

std::string Binding::toString() const
{
    return "__bind__" + variableName_;
}

std::string Binding::toTagStringId() const
{
    return "(" + variableName_ + " : " + iteratedType_ + ")";
}

bool Binding::operator==(const Binding& rhs) const
{
    return variableName_ == rhs.variableName_ && iteratedType_ == rhs.iteratedType_;
}

bool Binding::operator!=(const Binding& rhs) const
{
    return !this->operator==(rhs);
}

const std::string& Binding::getVariableName() const
{
    return variableName_;
}

const std::string& Binding::getTypeName() const
{
    return iteratedType_;
}

Node::Node(const nlohmann::json& t)
{
    name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

    if (const auto& optBinding = Parser::getPartFromParts(t, "Binding"))
    {
        const auto& binding = optBinding->get();
        assert(binding["type"]["kind"] == "TypeReference");
        binding_.emplace(binding["identifier"].get<std::string>(), binding["type"]["identifier"]);
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

// TODO: it's a temporary solution: use the same name in all places
std::string Node::getAlternativeName() const
{
    return binding_.has_value() ? name_ + binding_->toTagStringId() : name_;
}

const std::optional<Binding>& Node::getBinding() const
{
    return binding_;
}

bool Node::operator==(const Node& rhs) const
{
    return name_ == rhs.name_ && binding_ == rhs.binding_;
}
