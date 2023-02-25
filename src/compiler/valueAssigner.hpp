#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>


using SymbolToValueMap = std::map<std::string, int>;
using TypeToSymbolToValueMap = std::map<std::string, SymbolToValueMap>;

class ValueAssigner
{
public:
    void assignValuesToSymbols(const nlohmann::json& types);
    const TypeToSymbolToValueMap& getTypeToSymbolToValueMap() const;
    std::pair<int, int> getTypeMinMaxValues(const std::string& identifier) const;
    int getTypeRange(const std::string& identifier) const;
    int getTypeDomainSize(const std::string& identifier) const;

private:
    const SymbolToValueMap& getSymbolToValueMapForType(const std::string& identifier) const;
    void assignValuesForPlayers(const nlohmann::json& types);
    std::set<std::string> findSymbolsSharedAmongTypes(const nlohmann::json& types) const;

    std::map<std::string, int> symbolToValue_;
    TypeToSymbolToValueMap typeToSymbolToValue_;
};
