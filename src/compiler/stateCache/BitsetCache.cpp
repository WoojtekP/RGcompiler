#include "BitsetCache.hpp"

#include <string>
#include <vector>
#include <iostream>

#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>


BitsetCache::BitsetCache(
    const std::string& nodeName,
    const std::vector<std::string>& identifiers,
    const Parser& parser,
    const ValueAssigner& valueAssigner)
: cacheName_("state_cache_" + nodeName), identifiers_(identifiers), parser_(parser), valueAssigner_(valueAssigner)
{
}

std::string BitsetCache::getCacheType() const
{
    const auto sourceType = parser_.findTypeOfVariable(identifiers_.back());
    const auto typeRange = valueAssigner_.getTypeRange(sourceType["identifier"].get<std::string>());
    std::string cacheType = "std::bitset<" + std::to_string(typeRange) + ">";
    for (auto id = identifiers_.end() - 2; id >= identifiers_.begin(); --id)
    {
        const auto sourceType = parser_.findTypeOfVariable(*id);
        const auto typeRange = valueAssigner_.getTypeRange(sourceType["identifier"].get<std::string>());
        cacheType = "Arr<" + cacheType + "," + std::to_string(typeRange) + ">";
    }
    return cacheType;
}

std::string BitsetCache::getCacheName() const
{
    return cacheName_;
}

std::string BitsetCache::getInsertInstruction() const
{
    return getMethodCall("set");
}

std::string BitsetCache::getTestInstruction() const
{
    return getMethodCall("test");
}

std::string BitsetCache::getMethodCall(const std::string& method) const
{
    std::string expression;
    for (auto id = identifiers_.begin(); id < identifiers_.end() - 1; ++id)
    {
        expression += "[" + *id + "]";
    }
    return expression + "." + method + "(" + identifiers_.back() + ")";
}
