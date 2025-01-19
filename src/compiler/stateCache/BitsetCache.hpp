#pragma once

#include <string>
#include <vector>

#include <compiler/stateCache/IStateCache.hpp>
#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>


class BitsetCache : public IStateCache
{
public:
    BitsetCache(
        const std::string& nodeName,
        const std::vector<std::string>& identifiers,
        const Parser& parser,
        const ValueAssigner& valueAssigner);
    ~BitsetCache() = default;
    std::string getCacheType() const override;
    std::string getCacheName() const override;
    std::string getInsertInstruction() const override;
    std::string getTestInstruction() const override;
    std::string getResetInstruction() const override;

private:
    std::string getMethodCall(const std::string& method) const;
    std::map<std::string, std::pair<int, int>> getIdentifierToMultiplierAndOffsetMap() const;

    const std::string cacheName_;
    const std::vector<std::string> identifiers_;
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
};
