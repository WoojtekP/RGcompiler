#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

class IAction;
class Node;
class Parser;
class SymbolsManager;

struct NodesAndVariable
{
    std::vector<std::string> nodes;
    std::string variable;
    std::string iteratorName;
    std::string iteratorIndex;
};

class IteratorData
{
public:
    void parse(const Parser& parser, const SymbolsManager& symbolsManager);
    bool contains(const std::shared_ptr<Node>& lhsNode, const std::shared_ptr<Node>& rhsNode) const;
    bool contains(const std::shared_ptr<Node>& node) const;
    bool contains(const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& assignmentAnyAction) const;
    std::string getRangeName(
        const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& assignmentAnyAction) const;
    const std::set<std::string> getIteratorNames() const;
    const std::string getConstantNameForIterator(const std::string& iteratorName) const;

private:
    std::vector<NodesAndVariable> nodesAndVariables_;
    std::set<std::string> iteratorNames_;
};
