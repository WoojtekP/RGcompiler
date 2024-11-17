#pragma once

#include <memory>

#include <compiler/stateCache/IStateCache.hpp>
#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>


class StateCacheFactory
{
public:
    StateCacheFactory(const Parser& parser, const ValueAssigner& valueAssigner);
    std::shared_ptr<IStateCache> createStateCache(
        const std::string& state, const std::vector<std::string>& identifiers) const;

private:
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
};
