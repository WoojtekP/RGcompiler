#include <fstream>
#include <iomanip>
#include <iostream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>


namespace
{
void printValuesAssignmentDebugInfo(const std::map<std::string, int>& symbolToValue)
{
    std::map<int, std::string> valueToSymbol;
    for (const auto& [symbol, value] : symbolToValue)
    {
        valueToSymbol.emplace(value, symbol);
    }
    for (const auto& [value, symbol] : valueToSymbol)
    {
        std::cout << std::setw(3) <<  value << " : " << symbol << std::endl;
    }
}
}  // namespace


Parser::Parser(std::ifstream& jsonGameFile)
: parsedJson_(nlohmann::json::parse(jsonGameFile))
{
    int value = 0;
    for (const auto& el : parsedJson_["types"])
    {
        if (el["identifier"] == "Player")
        {
            continue;
        }
        if (el["type"]["kind"] == "Set")
        {
            for (const auto& id : el["type"]["identifiers"])
            {
                symbolToValue_.emplace(id, value++);
            }
        }
    }
    printValuesAssignmentDebugInfo(symbolToValue_);
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

std::string Parser::getValue(const std::string& symbol) const
{
    const auto symbolIt = symbolToValue_.find(symbol);
    if (symbolIt != symbolToValue_.end())
    {
        return std::to_string(symbolIt->second);
    }
    const auto symbolMatcher = [symbol](const auto& var)
    {
        return var["identifier"] == symbol;
    };
    if (std::any_of(parsedJson_["variables"].begin(), parsedJson_["variables"].end(), symbolMatcher))
    {
        return symbol;
    }
    if (std::any_of(parsedJson_["constants"].begin(), parsedJson_["constants"].end(), symbolMatcher))
    {
        return symbol;
    }
    return "?";
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

std::string Parser::getSourceType(const std::string& typeIdentifier) const
{
    const auto& t = findTypeByIdentifier(typeIdentifier);
    return t["type"]["lhs"];
}

std::string Parser::getDestinationType(const std::string& typeIdentifier) const
{
    const auto& t = findTypeByIdentifier(typeIdentifier);
    return t["type"]["rhs"]["identifier"];
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
