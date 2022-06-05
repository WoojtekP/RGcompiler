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
    std::string getValue(const std::string& symbol) const;
    std::vector<std::string> getDomain(const std::string& typeIdentifier) const;
    std::string getSourceType(const nlohmann::json& t) const;
    nlohmann::json getDestinationType(const nlohmann::json& t) const;
    nlohmann::json findTypeByIdentifier(const std::string& typeIdentifier) const;

private:
    nlohmann::json parsedJson_;
    std::map<std::string, int> symbolToValue_;
};
