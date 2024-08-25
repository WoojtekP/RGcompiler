#pragma once

#include <memory>
#include <string>
#include <vector>

class ILoopInstruction;
class Node;
class Binding;
class Parser;
class ValueAssigner;

class LoopFactory
{
public:
    LoopFactory(const Parser& parser, const ValueAssigner& valueAssigner);

    std::unique_ptr<ILoopInstruction> createLoopInstruction(const Binding& binding) const;

private:
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
};
