#include "LoopFactory.hpp"

#include <compiler/ValueAssigner.hpp>
#include <compiler/pragma/Iterator.hpp>
#include <graph/Action.hpp>
#include <graph/Node.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

LoopFactory::LoopFactory(
    const Parser& parser, const ValueAssigner& valueAssigner, const IteratorData& pragmaIteratorData)
: parser_(parser)
, valueAssigner_(valueAssigner)
, pragmaIteratorData_(pragmaIteratorData)
{}

std::unique_ptr<ILoopInstruction> LoopFactory::createLoopInstruction(
    const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& action) const
{
    if (action->getType() != ActionType::AssignmentAny)
    {
        throw std::invalid_argument("[LoopFactory] Loop cannot be created for action: " + action->toString());
    }
    const auto& typeName = action->getRightSide();
    const auto iteratorVariableName = action->getLeftSide() + "It";
    if (pragmaIteratorData_.isIteratorAction(node, action))
    {
        auto loopInstruction = std::make_unique<RangeLoopInstruction>(iteratorVariableName);
        loopInstruction->setRange(pragmaIteratorData_.getRangeName(node, action));
        return std::move(loopInstruction);
    }
    else if (valueAssigner_.getTypeDomainSize(typeName) == valueAssigner_.getTypeRange(typeName))
    {
        const auto [minSymbol, maxSymbol] = valueAssigner_.getTypeMinMaxSymbols(typeName);
        return std::make_unique<IterLoopInstruction>(action->getLeftSide(), minSymbol, maxSymbol);
    }
    else
    {
        auto loopInstruction = std::make_unique<RangeLoopInstruction>(iteratorVariableName);
        loopInstruction->setRange(parser_.getDomain(typeName));
        return std::move(loopInstruction);
    }
}
