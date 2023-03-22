#include "ValueAssigner.hpp"

#include <string>

#include <nlohmann/json.hpp>

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

ValueAssigner::ValueAssigner(const nlohmann::json& types)
{
    assignValuesToSymbols(types);
}

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

int ValueAssigner::getTypeRange(const std::string& identifier) const
{
    const auto [minValue, maxValue] = getTypeMinMaxValues(identifier);
    return maxValue - minValue + 1;
}

int ValueAssigner::getTypeDomainSize(const std::string& identifier) const
{
    return getSymbolToValueMapForType(identifier).size();
}

void ValueAssigner::assignValuesToSymbols(const nlohmann::json& types)
{
    typeToSymbolToValue_.clear();

    SymbolToTypesMap reservedValuesPerType;
    assignValuesForNumbers(reservedValuesPerType, types);
    assignValuesForPlayers(reservedValuesPerType, types);
    assignValuesForSharedSymbols(reservedValuesPerType, types);
    assignValuesForRemainingSymbols(reservedValuesPerType, types);
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

void ValueAssigner::assignValuesForNumbers(SymbolToTypesMap& reservedValuesPerType, const nlohmann::json& types)
{
    for (const auto& el : types)
    {
        const auto typeName = el["identifier"].get<std::string>();
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrKeeper")
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
            auto& reservedValuesForPlayersOrKeeper = reservedValuesPerType["PlayerOrKeeper"];
            typeToSymbolToValue_["PlayerOrKeeper"].emplace("keeper", 0);
            reservedValuesForPlayersOrKeeper.insert(0);
            int value = 1;
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
                typeToSymbolToValue_["Player"].emplace(symbol, value);
                typeToSymbolToValue_["PlayerOrKeeper"].emplace(symbol, value);
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
    std::map<std::string, std::string> symbolToType;
    std::map<std::string, std::set<std::string>> commonSymbolToTypes;
    for (const auto& el : types)
    {
        const auto typeName = el["identifier"].get<std::string>();
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrKeeper")
        {
            for (const auto& identifier : el["type"]["identifiers"])
            {
                const auto symbol = identifier.get<std::string>();
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
        while (isValueReserved(value, types, reservedValuesPerType))
        {
            ++value;
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
        if (el["type"]["kind"] == "Set" && typeName != "Player" && typeName != "PlayerOrKeeper")
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
