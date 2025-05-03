#include "LoopFactory.hpp"

#include <compiler/ValueAssigner.hpp>
#include <graph/Action.hpp>
#include <graph/Node.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

LoopFactory::LoopFactory(const Parser& parser, const ValueAssigner& valueAssigner)
: parser_(parser)
, valueAssigner_(valueAssigner)
{}

std::unique_ptr<ILoopInstruction> LoopFactory::createLoopInstruction(const IAction& action) const
{
    if (action.getType() != ActionType::AssignmentAny)
    {
        throw std::invalid_argument("[LoopFactory] Loop cannot be created for action: " + action.toString());
    }
    const auto& iteratorVariableName = action.getLeftSide() + "It";
    const auto& typeName = action.getRightSide();
    if (valueAssigner_.getTypeDomainSize(typeName) == valueAssigner_.getTypeRange(typeName))
    {
        const auto [minSymbol, maxSymbol] = valueAssigner_.getTypeMinMaxSymbols(typeName);
        return std::make_unique<IterLoopInstruction>(action.getLeftSide(), minSymbol, maxSymbol);
    }
    auto loopInstruction = std::make_unique<RangeLoopInstruction>(iteratorVariableName);
    loopInstruction->setRange(parser_.getDomain(typeName));
    return std::move(loopInstruction);
}
