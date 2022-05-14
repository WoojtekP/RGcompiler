#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <nlohmann/json.hpp>


class Parser
{
public:
    Parser(std::ifstream& jsonGameFile);
    nlohmann::json getTypeDeclarations() const;
    nlohmann::json getVariables() const;
    nlohmann::json getConstants() const;
    int getValue(const std::string& symbol) const;

private:
    nlohmann::json parsedJson_;
    std::map<std::string, int> symbolToValue_;
};
