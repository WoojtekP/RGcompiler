#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>
#include <program/program.hpp>


Parser::Parser(std::ifstream& jsonGameFile) : parsedJson_(nlohmann::json::parse(jsonGameFile))
{
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
        return getDomain(t["type"]["lhs"]);
    }
    return {};
}

std::string Parser::getSourceType(const nlohmann::json& t) const
{
    if (t.is_string())
    {
        const auto& typeObject = findTypeByIdentifier(t);
        return typeObject["type"]["lhs"];
    }
    if (t["kind"] == "TypeReference")
    {
        const auto& typeObject = findTypeByIdentifier(t["identifier"]);
        return typeObject["type"]["lhs"];
    }
    else if (t["kind"] == "Arrow")
    {
        return t["lhs"];
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

nlohmann::json Parser::findTypeByIdentifier(const std::string& typeIdentifier) const
{
    for (const auto& el : parsedJson_["types"])
    {
        if (el["identifier"] == typeIdentifier)
        {
            return el;
        }
    }
    throw std::invalid_argument("Cannot found type identifier: " + typeIdentifier);
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
