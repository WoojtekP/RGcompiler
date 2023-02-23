#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>


using TypeToSymbolToValueMap = std::map<std::string, std::map<std::string, int>>;

class ValueAssigner
{
public:
    void assignValuesToSymbols(const nlohmann::json& types);
    const TypeToSymbolToValueMap& getTypeToSymbolToValueMap() const;

private:
    void assignValuesForPlayers(const nlohmann::json& types);
    std::set<std::string> findSymbolsSharedAmongTypes(const nlohmann::json& types) const;

    std::map<std::string, int> symbolToValue_;
    TypeToSymbolToValueMap typeToSymbolToValue_;
};
