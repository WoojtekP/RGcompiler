#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>


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

int Parser::getValue(const std::string& symbol) const
{
    const auto symbolIt = symbolToValue_.find(symbol);
    if (symbolIt == symbolToValue_.end())
    {
        throw std::invalid_argument("unknown value for symbol: " + symbol);
    }
    return symbolIt->second;
}
