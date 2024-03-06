#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

class Binding;
class Edge;

using SymbolToValueMap = std::map<std::string, int>;
using TypeToSymbolToValueMap = std::map<std::string, SymbolToValueMap>;
using EdgesWithIID = std::vector<std::pair<std::shared_ptr<Edge>, int>>;

class ValueAssigner
{
public:
    const TypeToSymbolToValueMap& getTypeToSymbolToValueMap() const;
    std::pair<int, int> getTypeMinMaxValues(const std::string& identifier) const;
    int getTypeRange(const std::string& identifier) const;
    int getTypeDomainSize(const std::string& identifier) const;
    int getBaseValueForTag(const std::string& tag) const;
    void assignValuesForSymbols(const nlohmann::json& types);
    void assignValuesForTags(const EdgesWithIID& edgesWithIid);

private:
    using SymbolToTypesMap = std::map<std::string, std::set<int>>;

    const SymbolToValueMap& getSymbolToValueMapForType(const std::string& identifier) const;
    void assignValuesForPlayers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForNumbers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForSharedSymbols(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    void assignValuesForRemainingSymbols(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types);
    int assignValueForTagFromBinding(const std::optional<Binding>& binding, int nextTagValue);

    TypeToSymbolToValueMap typeToSymbolToValue_;
    SymbolToValueMap tagToBaseValue_;
};
