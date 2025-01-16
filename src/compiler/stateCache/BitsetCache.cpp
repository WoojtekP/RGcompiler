#include "BitsetCache.hpp"

#include <string>
#include <vector>
#include <ranges>

#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>


namespace
{
std::string getIndexExpressionPart(const std::string& identifier, const int multiplier, const int offset)
{
    std::string expression = (offset == 0) ? identifier : "(" + identifier + "-" + std::to_string(offset) + ")";
    if (multiplier > 1)
    {
        expression += "*" + std::to_string(multiplier);
    }
    return expression;
}
}

BitsetCache::BitsetCache(
    const std::string& nodeName,
    const std::vector<std::string>& identifiers,
    const Parser& parser,
    const ValueAssigner& valueAssigner)
: cacheName_("state_cache_" + nodeName)
, identifiers_(identifiers)
, parser_(parser)
, valueAssigner_(valueAssigner)
{}

std::string BitsetCache::getCacheType() const
{
    auto combinedRangeSize = 1;
    for (const auto& identifier : identifiers_)
    {
        const auto sourceType = parser_.findTypeOfVariable(identifier);
        const auto typeRange = valueAssigner_.getTypeRange(sourceType["identifier"].get<std::string>());
        combinedRangeSize *= typeRange;
    }
    return combinedRangeSize == 1 ? "bool" : "std::bitset<" + std::to_string(combinedRangeSize) + ">";
}

std::string BitsetCache::getCacheName() const
{
    return cacheName_;
}

std::string BitsetCache::getInsertInstruction() const
{
    if (identifiers_.empty())
    {
        return " = true";
    }
    return getMethodCall("set");
}

std::string BitsetCache::getTestInstruction() const
{
    if (identifiers_.empty())
    {
        return "";
    }
    return getMethodCall("test");
}

std::string BitsetCache::getMethodCall(const std::string& method) const
{
    if (identifiers_.size() == 1)
    {
        return "." + method + "(" + identifiers_.front() + ")";
    }
    const auto identifierToMultiplierAndOffsetMap = getIdentifierToMultiplierAndOffsetMap();
    std::string indexExpression;
    for (const auto& identifier : identifiers_)
    {
        const auto [multiplier, offset] = identifierToMultiplierAndOffsetMap.at(identifier);
        indexExpression += getIndexExpressionPart(identifier, multiplier, offset) + "+";
    }
    indexExpression.pop_back();
    return "." + method + "(" + indexExpression + ")";
}

std::map<std::string, std::pair<int, int>> BitsetCache::getIdentifierToMultiplierAndOffsetMap() const
{
    std::map<std::string, std::pair<int, int>> identifierToMultiplierAndOffsetMap;
    int currentMultiplier = 1;
    for (const auto& identifier : std::ranges::reverse_view(identifiers_))
    {
        const auto sourceType = parser_.findTypeOfVariable(identifier);
        const auto [minValue, maxValue] = valueAssigner_.getTypeMinMaxValues(sourceType["identifier"].get<std::string>());
        const auto typeRange = maxValue - minValue + 1;
        identifierToMultiplierAndOffsetMap.emplace(identifier, std::make_pair(currentMultiplier, minValue));
        currentMultiplier *= typeRange;
    }
    return identifierToMultiplierAndOffsetMap;
}
