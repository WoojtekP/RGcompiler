#include "ValueAssigner.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include <graph/Action.hpp>
#include <graph/Edge.hpp>
#include <parser/Parser.hpp>

namespace
{
bool isNumber(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), ::isdigit);
}

bool isValueReserved(
    const int value,
    const std::set<std::string>& types,
    const std::map<std::string, std::set<int>>& reservedValuesPerType)
{
    return std::any_of(types.begin(), types.end(), [&](const auto& type) {
        return reservedValuesPerType.count(type) && reservedValuesPerType.at(type).count(value);
    });
}
}  // namespace

const TypeToSymbolToValueMap& ValueAssigner::getTypeToSymbolToValueMap() const
{
    return typeToSymbolToValue_;
}

std::pair<int, int> ValueAssigner::getTypeMinMaxValues(const std::string& identifier) const
{
    const auto& symbolToValuesMap = getSymbolToValueMapForType(identifier);
    const auto [minIt, maxIt] =
        std::minmax_element(symbolToValuesMap.begin(), symbolToValuesMap.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second < rhs.second;
        });

    return std::make_pair(minIt->second, maxIt->second);
}

std::pair<std::string, std::string> ValueAssigner::getTypeMinMaxSymbols(const std::string& identifier) const
{
    const auto& symbolToValuesMap = getSymbolToValueMapForType(identifier);
    const auto [minIt, maxIt] =
        std::minmax_element(symbolToValuesMap.begin(), symbolToValuesMap.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second < rhs.second;
        });

    return std::make_pair(minIt->first, maxIt->first);
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

int ValueAssigner::getBaseValueForTag(const std::string& tag) const
{
    return getRangeValueForTag(tag).first;
}

std::pair<int, int> ValueAssigner::getRangeValueForTag(const std::string& tag) const
{
    const auto tagToValueIt = tagToValues_.find(tag);
    if (tagToValueIt == tagToValues_.end())
    {
        throw std::runtime_error("[ValueAssigner] Unknown tag: " + tag);
    }
    return tagToValueIt->second;
}

void ValueAssigner::assignValuesForSymbols(const nlohmann::json& types)
{
    typeToSymbolToValue_.clear();

    SymbolToTypesMap reservedValuesPerType;
    assignValuesForNumbers(reservedValuesPerType, types);
    assignValuesForPlayers(reservedValuesPerType, types);
    assignValuesForSharedSymbols(reservedValuesPerType, types);
    assignValuesForRemainingSymbols(reservedValuesPerType, types);
}

void ValueAssigner::assignValuesForTags(const Parser& parser, const nlohmann::json& edges)
{
    if (typeToSymbolToValue_.empty())
    {
        throw std::runtime_error("[ValueAssigner] Values for symbols should be assigned before tags");
    }
    tagToValues_.clear();

    int nextTagValue = 0;
    for (const auto& edge : edges)
    {
        const auto& label = edge["label"];
        const auto& labelKind = label["kind"].get<std::string>();
        if (labelKind == "Tag")
        {
            nextTagValue = assignValueForSimpleTag(label["symbol"].get<std::string>(), nextTagValue);
        }
        else if (labelKind == "TagVariable")
        {
            nextTagValue = assignValueForTagVariable(parser, label["identifier"].get<std::string>(), nextTagValue);
        }
    }
}

const SymbolToValueMap& ValueAssigner::getSymbolToValueMapForType(const std::string& identifier) const
{
    const auto typeToSymbolToValueIt = typeToSymbolToValue_.find(identifier);
    if (typeToSymbolToValueIt == typeToSymbolToValue_.end())
    {
        throw std::runtime_error("[ValueAssigner] Unknown type: " + identifier);
    }
    return typeToSymbolToValueIt->second;
}

void ValueAssigner::assignValuesForNumbers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types)
{
    for (const auto& el : types)
    {
        const auto typeName = el["identifier"].get<std::string>();
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrSystem")
        {
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
                if (isNumber(symbol))
                {
                    const auto value = std::stoi(symbol);
                    typeToSymbolToValue_[typeName].emplace(symbol, value);
                    reservedValuesPerType[typeName].insert(value);
                }
            }
        }
    }
}

void ValueAssigner::assignValuesForPlayers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types)
{
    for (const auto& el : types)
    {
        if (el["identifier"] == "Player")
        {
            auto& reservedValuesForPlayers = reservedValuesPerType["Player"];
            auto& reservedValuesForPlayersOrKeeper = reservedValuesPerType["PlayerOrSystem"];
            typeToSymbolToValue_["PlayerOrSystem"].emplace("random", -1);
            typeToSymbolToValue_["PlayerOrSystem"].emplace("keeper", 0);
            reservedValuesForPlayersOrKeeper.insert(0);
            int value = 1;
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
                typeToSymbolToValue_["Player"].emplace(symbol, value);
                typeToSymbolToValue_["PlayerOrSystem"].emplace(symbol, value);
                reservedValuesForPlayers.insert(value);
                reservedValuesForPlayersOrKeeper.insert(value);
                value++;
            }
            return;
        }
    }
    throw std::runtime_error("[ValueAssigner] 'Player' type is not available!");
}

void ValueAssigner::assignValuesForSharedSymbols(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types)
{
    std::map<std::string, std::set<std::string>> commonSymbolToTypes;
    std::map<std::string, std::string> symbolToType;
    for (const auto& [symbol, _] : typeToSymbolToValue_["PlayerOrSystem"])
    {
        symbolToType.emplace(symbol, "PlayerOrSystem");
    }
    for (const auto& el : types)
    {
        const auto typeName = el["identifier"].get<std::string>();
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrSystem")
        {
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
                if (isNumber(symbol))
                {
                    continue;
                }
                if (symbolToType.count(symbol))
                {
                    commonSymbolToTypes[symbol].insert(typeName);
                    commonSymbolToTypes[symbol].insert(symbolToType[symbol]);
                }
                else
                {
                    symbolToType.emplace(symbol, typeName);
                }
            }
        }
    }

    for (const auto& [symbol, types] : commonSymbolToTypes)
    {
        int value = 0;
        if (const auto valueAssignedToPlayer = getValueIfAssignedForPlayer(symbol))
        {
            value = *valueAssignedToPlayer;
        }
        else
        {
            while (isValueReserved(value, types, reservedValuesPerType))
            {
                ++value;
            }
        }
        for (const auto& type : types)
        {
            typeToSymbolToValue_[type].emplace(symbol, value);
            reservedValuesPerType[type].insert(value);
        }
    }
}

void ValueAssigner::assignValuesForRemainingSymbols(
    SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types)
{
    for (const auto& el : types)
    {
        const auto typeName = el["identifier"].get<std::string>();
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrSystem")
        {
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
                if (!typeToSymbolToValue_[typeName].count(symbol))
                {
                    int value = 0;
                    while (reservedValuesPerType.count(typeName) && reservedValuesPerType.at(typeName).count(value))
                    {
                        ++value;
                    }
                    typeToSymbolToValue_[typeName].emplace(symbol, value);
                    reservedValuesPerType[typeName].insert(value);
                }
            }
        }
    }
}

int ValueAssigner::assignValueForTagVariable(const Parser& parser, const std::string& tagString, int nextTagValue)
{
    const auto tagType = parser.findTypeOfVariable(tagString)["identifier"].get<std::string>();
    if (tagToValues_.count(tagType))
    {
        tagToValues_[tagString] = tagToValues_[tagType];
        return nextTagValue;
    }
    const auto [minTypeValue, maxTypeValue] = getTypeMinMaxValues(tagType);
    const auto minTagValue = nextTagValue + minTypeValue;
    const auto maxTagValue = nextTagValue + maxTypeValue;
    tagToValues_.emplace(tagType, std::make_pair(minTagValue, maxTagValue));
    tagToValues_.emplace(tagString, std::make_pair(minTagValue, maxTagValue));
    return maxTagValue + 1;
}

int ValueAssigner::assignValueForSimpleTag(const std::string& symbol, int nextTagValue)
{
    if (tagToValues_.emplace(symbol, std::make_pair(nextTagValue, nextTagValue)).second)
    {
        return nextTagValue + 1;
    }
    return nextTagValue;
}

std::optional<int> ValueAssigner::getValueIfAssignedForPlayer(const std::string& symbol) const
{
    const auto playerSymbolToValueIt = typeToSymbolToValue_.find("PlayerOrSystem");
    if (playerSymbolToValueIt != typeToSymbolToValue_.end())
    {
        const auto symbolToValueIt = playerSymbolToValueIt->second.find(symbol);
        if (symbolToValueIt != playerSymbolToValueIt->second.end())
        {
            return symbolToValueIt->second;
        }
    }
    return std::nullopt;
}
