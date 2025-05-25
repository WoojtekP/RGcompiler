#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>

class Edge;
class IAction;

using SymbolToValueMap = std::map<std::string, int>;
using SymbolToValueRangeMap = std::map<std::string, std::pair<int, int>>;
using TypeToSymbolToValueMap = std::map<std::string, SymbolToValueMap>;
using EdgesWithIID = std::vector<std::pair<std::shared_ptr<Edge>, int>>;

class ValueAssigner
{
public:
    const TypeToSymbolToValueMap& getTypeToSymbolToValueMap() const;
    std::pair<int, int> getTypeMinMaxValues(const std::string& identifier) const;
    std::pair<std::string, std::string> getTypeMinMaxSymbols(const std::string& identifier) const;
    int getTypeRange(const std::string& identifier) const;
    int getTypeDomainSize(const std::string& identifier) const;
    int getBaseValueForTag(const std::string& tag) const;
    std::pair<int, int> getRangeValueForTag(const std::string& tag) const;
    void assignValuesForSymbols(const SymbolToValueMap& integerSymbolToValue, const nlohmann::json& types);
    void assignValuesForTags(const Parser& parser, const nlohmann::json& edges);

private:
    using SymbolToTypesMap = std::map<std::string, std::set<int>>;

    const SymbolToValueMap& getSymbolToValueMapForType(const std::string& identifier) const;
    void assignValuesForPlayers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForNumbers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForSharedSymbols(
        SymbolToTypesMap& reservedValuesPerType, const SymbolToValueMap& integerSymbolToValue, const nlohmann::json& types);
    void assignValuesForRemainingSymbols(
        SymbolToTypesMap& reservedValuesPerType, const SymbolToValueMap& integerSymbolToValue, const nlohmann::json& types);
    int assignValueForTagVariable(const Parser& parser, const std::string& identifier, int nextTagValue);
    int assignValueForSimpleTag(const std::string& symbol, int nextTagValue);
    std::optional<int> getValueIfAssignedForPlayer(const std::string& symbol) const;

    TypeToSymbolToValueMap typeToSymbolToValue_;
    SymbolToValueRangeMap tagToValues_;
};
