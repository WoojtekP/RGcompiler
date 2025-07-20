#pragma once

#include <memory>
#include <string>
#include <vector>

class ILoopInstruction;
class Node;
class Parser;
class IteratorData;
class ValueAssigner;
class IAction;

class LoopFactory
{
public:
    LoopFactory(const Parser& parser, const ValueAssigner& valueAssigner, const IteratorData& pragmaIteratorData);

    std::unique_ptr<ILoopInstruction> createLoopInstruction(
        const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& action) const;

private:
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
    const IteratorData& pragmaIteratorData_;
};
