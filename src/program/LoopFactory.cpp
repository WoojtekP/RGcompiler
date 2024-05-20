#include "LoopFactory.hpp"

#include <compiler/ValueAssigner.hpp>
#include <graph/Node.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

LoopFactory::LoopFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
{}

std::unique_ptr<ILoopInstruction> LoopFactory::createLoopInstruction(const Binding& binding) const
{
    const auto& iteratorVariableName = binding.getVariableName();
    const auto& typeName = binding.getTypeName();
    if (valueAssigner_.getTypeDomainSize(typeName) == valueAssigner_.getTypeRange(typeName))
    {
        const auto [minSymbol, maxSymbol] = valueAssigner_.getTypeMinMaxSymbols(typeName);
        return std::make_unique<IterLoopInstruction>(iteratorVariableName, minSymbol, maxSymbol);
    }
    auto loopInstruction = std::make_unique<RangeLoopInstruction>(iteratorVariableName);
    loopInstruction->setRange(parser_.getDomain(typeName));
    return std::move(loopInstruction);
}