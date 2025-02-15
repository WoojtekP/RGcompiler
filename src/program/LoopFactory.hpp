#pragma once

#include <memory>
#include <string>
#include <vector>

class ILoopInstruction;
class Node;
class Binding;
class Parser;
class ValueAssigner;
class IAction;

class LoopFactory
{
public:
    LoopFactory(const Parser& parser, const ValueAssigner& valueAssigner);

    std::unique_ptr<ILoopInstruction> createLoopInstruction(const IAction& action) const;

private:
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
};
