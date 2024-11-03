#include "StateCacheFactory.hpp"

#include <compiler/stateCache/BitsetCache.hpp>
#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>


StateCacheFactory::StateCacheFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
{}

std::shared_ptr<IStateCache> StateCacheFactory::createStateCache(
    const std::string& state, const std::vector<std::string>& identifiers) const
{
   return std::make_shared<BitsetCache>(state, identifiers, parser_, valueAssigner_);
}
