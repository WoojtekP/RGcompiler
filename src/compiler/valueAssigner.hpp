#pragma once

#include <map>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

using SymbolToValueMap = std::map<std::string, int>;
using TypeToSymbolToValueMap = std::map<std::string, SymbolToValueMap>;

class ValueAssigner
{
public:
    ValueAssigner(const nlohmann::json& types);
    const TypeToSymbolToValueMap& getTypeToSymbolToValueMap() const;
    std::pair<int, int> getTypeMinMaxValues(const std::string& identifier) const;
    int getTypeRange(const std::string& identifier) const;
    int getTypeDomainSize(const std::string& identifier) const;

private:
    using SymbolToTypesMap = std::map<std::string, std::set<int>>;

    void assignValuesToSymbols(const nlohmann::json& types);
    const SymbolToValueMap& getSymbolToValueMapForType(const std::string& identifier) const;
    void assignValuesForPlayers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForNumbers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForSharedSymbols(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForRemainingSymbols(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);

    TypeToSymbolToValueMap typeToSymbolToValue_;
};
