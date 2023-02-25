#include <set>
#include <string>

#include "valueAssigner.hpp"

#include <nlohmann/json.hpp>


namespace
{
bool isNumber(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), ::isdigit);
}
}  // namespace

void ValueAssigner::assignValuesToSymbols(const nlohmann::json& types)
{
    symbolToValue_.clear();
    typeToSymbolToValue_.clear();

    assignValuesForPlayers(types);

    const auto commonSymbols = findSymbolsSharedAmongTypes(types);
    for (const auto& el : types)
    {
        if (el["type"]["kind"] == "Set" && el["identifier"] != "Player" && el["identifier"] != "PlayerOrKeeper")
        {
            const std::string& typeName = el["identifier"].get<std::string>();
            if (std::all_of(el["type"]["identifiers"].begin(), el["type"]["identifiers"].end(), isNumber))
            {
                for (const auto& identifier : el["type"]["identifiers"])
                {
                    const std::string id = identifier.get<std::string>();
                    typeToSymbolToValue_[typeName].emplace(id, std::stoi(id));
                }
                continue;
            }
            assert(!std::any_of(el["type"]["identifiers"].begin(), el["type"]["identifiers"].end(), isNumber));

            int value = 0;
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const std::string id = identifier.get<std::string>();
                if (commonSymbols.count(id))
                {
                    if (!symbolToValue_.count(id))
                    {
                        symbolToValue_.emplace(id, value);
                    }
                    typeToSymbolToValue_[typeName].emplace(id, symbolToValue_[id]);
                    value++;
                }
            }

            for (const auto& identifier : el["type"]["identifiers"])
            {
                const std::string id = identifier.get<std::string>();
                if (!commonSymbols.count(id))
                {
                    symbolToValue_.emplace(id, value);
                    typeToSymbolToValue_[typeName].emplace(id, value);
                    value++;
                }
            }

            std::set<int> assignedValues;
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const std::string id = identifier.get<std::string>();
                assignedValues.insert(symbolToValue_[id]);
            }
            assert(assignedValues.size() == el["type"]["identifiers"].size());
        }
    }
}

const TypeToSymbolToValueMap& ValueAssigner::getTypeToSymbolToValueMap() const
{
    return typeToSymbolToValue_;
}

std::pair<int, int> ValueAssigner::getTypeMinMaxValues(const std::string& identifier) const
{
    const auto& symbolToValuesMap = getSymbolToValueMapForType(identifier);
    const auto [minIt, maxIt] = std::minmax_element(
        symbolToValuesMap.begin(),
        symbolToValuesMap.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    return std::make_pair(minIt->second, maxIt->second);
}

int ValueAssigner::getTypeRange(const std::string& identifier) const
{
    const auto [minValue, maxValue] = getTypeMinMaxValues(identifier);
    return maxValue - minValue + 1;
}

int ValueAssigner::getTypeDomainSize(const std::string& identifier) const
{
    return getSymbolToValueMapForType(identifier).size();
}

const SymbolToValueMap& ValueAssigner::getSymbolToValueMapForType(const std::string& identifier) const
{
    const auto symbolToValuesIt = typeToSymbolToValue_.find(identifier);
    if (symbolToValuesIt == typeToSymbolToValue_.end())
    {
        throw std::runtime_error("[ValueAssigner] Unknown type: " + identifier);
    }
    return symbolToValuesIt->second;
}

void ValueAssigner::assignValuesForPlayers(const nlohmann::json& types)
{
    for (const auto& el : types)
    {
        if (el["identifier"] == "Player")
        {
            symbolToValue_.emplace("keeper", 0);
            typeToSymbolToValue_["PlayerOrKeeper"].emplace("keeper", 0);
            int value = 1;
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const std::string id = identifier.get<std::string>();
                symbolToValue_.emplace(id, value);
                typeToSymbolToValue_["Player"].emplace(id, value);
                typeToSymbolToValue_["PlayerOrKeeper"].emplace(id, value);
                value++;
            }
            return;
        }
    }
    throw std::runtime_error("[ValueAssigner] 'Player' type is not available!");
}

std::set<std::string> ValueAssigner::findSymbolsSharedAmongTypes(const nlohmann::json& types) const
{
    std::set<std::string> allSymbols;
    std::set<std::string> commonSymbols;
    for (const auto& el : types)
    {
        if (el["type"]["kind"] == "Set" && el["identifier"] != "Player" && el["identifier"] != "PlayerOrKeeper")
        {
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const std::string id = identifier.get<std::string>();
                if (allSymbols.count(id))
                {
                    commonSymbols.insert(id);
                }
                else
                {
                    allSymbols.insert(id);
                }
            }
        }
    }
    return allSymbols;
}
