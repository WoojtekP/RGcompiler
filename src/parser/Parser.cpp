#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>
#include <program/Program.hpp>


Parser::Parser(std::ifstream& jsonGameFile) : parsedJson_(nlohmann::json::parse(jsonGameFile))
{
    for (const auto& type : getTypeDeclarations())
    {
        if (type["type"]["kind"] == "Set")
        {
            for (const auto& symbol : type["type"]["identifiers"])
            {
                symbols_.insert(symbol.get<std::string>());
            }
        }
    }

    for (const auto& constant : getConstants())
    {
        constants_.insert(constant["identifier"].get<std::string>());
    }

    for (const auto& variable : getVariables())
    {
        variables_.insert(variable["identifier"].get<std::string>());
    }
}

bool Parser::isSymbol(const std::string& identifier) const
{
    return symbols_.count(identifier);
}

bool Parser::isConstant(const std::string& identifier) const
{
    return constants_.count(identifier);
}

bool Parser::isVariable(const std::string& identifier) const
{
    return variables_.count(identifier);
}

nlohmann::json Parser::getTypeDeclarations() const
{
    return parsedJson_["types"];
}

nlohmann::json Parser::getVariables() const
{
    return parsedJson_["variables"];
}

nlohmann::json Parser::getConstants() const
{
    return parsedJson_["constants"];
}

nlohmann::json Parser::getEdges() const
{
    return parsedJson_["edges"];
}

std::vector<std::string> Parser::getDomain(const std::string& typeIdentifier) const
{
    const auto& t = findTypeByIdentifier(typeIdentifier);
    if (t["type"]["kind"] == "Set")
    {
        return t["type"]["identifiers"];
    }
    else if (t["type"]["kind"] == "Arrow")
    {
        return getDomain(t["type"]["lhs"]["identifier"]);
    }
    return {};
}

std::string Parser::getSourceType(const nlohmann::json& t) const
{
    if (t.is_string())
    {
        const auto& typeObject = findTypeByIdentifier(t);
        return typeObject["type"]["lhs"]["identifier"];
    }
    if (t["kind"] == "TypeReference")
    {
        const auto& typeObject = findTypeByIdentifier(t["identifier"]);
        return typeObject["type"]["lhs"]["identifier"];
    }
    else if (t["kind"] == "Arrow")
    {
        return t["lhs"]["identifier"];
    }
    return "?";
}

nlohmann::json Parser::getDestinationType(const nlohmann::json& t) const
{
    if (t.is_string())
    {
        const auto& typeObject = findTypeByIdentifier(t);
        return typeObject["type"]["rhs"]["identifier"];
    }
    if (t["kind"] == "TypeReference")
    {
        const auto& typeObject = findTypeByIdentifier(t["identifier"]);
        return typeObject["type"]["rhs"]["identifier"];
    }
    else if (t["kind"] == "Arrow")
    {
        return t["rhs"];
    }
    return "?";
}

nlohmann::json Parser::findTypeOfExpression(const nlohmann::json& expression) const
{
    const auto& expressionKind = expression["kind"].get<std::string>();
    if (expressionKind == "Reference")
    {
        return findTypeOfVariable(expression["identifier"]);
    }
    if (expressionKind == "TypeReference")
    {
        return findTypeByIdentifier(expression["identifier"]);
    }
    if (expressionKind == "Access")
    {
        return getDestinationType(findTypeOfExpression(expression["lhs"]));
    }
    if (expressionKind == "Cast")
    {
        return findTypeOfExpression(expression["lhs"]);
    }
    if (expressionKind == "EdgeName")
    {
        throw std::runtime_error("[Parser] Illegal operation: cannot extract type from edge.");
    }
    throw std::runtime_error("[ExpressionFactory] Unknown type of expression " + expressionKind);
}

nlohmann::json Parser::findTypeOfVariable(const std::string& identifier) const
{
    if (variables_.count(identifier))
    {
        for (const auto& variable : getVariables())
        {
            if (variable["identifier"] == identifier)
            {
                return variable["type"];
            }
        }
        throw std::runtime_error("[Parser] Cannot find type of variable: " + identifier);
    }
    if (constants_.count(identifier))
    {
        for (const auto& constant : getConstants())
        {
            if (constant["identifier"] == identifier)
            {
                return constant["type"];
            }
        }
        throw std::runtime_error("[Parser] Cannot find type of constant: " + identifier);
    }
    throw std::runtime_error("[Parser] Unknown variable or constant: " + identifier);
}

nlohmann::json Parser::findTypeByIdentifier(const std::string& typeIdentifier) const
{
    for (const auto& el : parsedJson_["types"])
    {
        if (el["identifier"] == typeIdentifier)
        {
            return el;
        }
    }
    throw std::invalid_argument("Cannot find type identifier: " + typeIdentifier);
}

std::string Parser::getValueFromEntries(
    const nlohmann::json& entries, const std::string& entryKind, const std::string& entryName)
{
    for (const auto& entry : entries)
    {
        if (entry["kind"] == entryKind)
        {
            return entry[entryName];
        }
    }

    return "?";
}

std::optional<std::reference_wrapper<const nlohmann::json>> Parser::getPartFromParts(
    const nlohmann::json& parts, const std::string& entryKind)
{
    for (const auto& entry : parts)
    {
        if (entry["kind"] == entryKind)
        {
            return entry;
        }
    }

    return std::nullopt;
}
