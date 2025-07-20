#include "Iterator.hpp"

#include <iostream>

#include <compiler/SymbolsManager.hpp>
#include <compiler/ValueFactory.hpp>
#include <graph/Action.hpp>
#include <graph/Node.hpp>
#include <parser/Parser.hpp>

namespace
{
constexpr std::string_view ITERATOR_SUFFIX = "iter";
}  // namespace

void IteratorData::parse(const Parser& parser, const SymbolsManager& symbolsManager)
{
    for (const auto pragma : parser.getPragmas("Iterator"))
    {
        const std::vector<std::string> nodes {
            pragma["node_a"]["identifier"].get<std::string>(),
            pragma["node_b"]["identifier"].get<std::string>(),
            pragma["node_c"]["identifier"].get<std::string>(),
        };
        const auto variable = pragma["variable"].get<std::string>();

        std::string constantMap = "";
        std::string iteratorIndex = "";
        const auto& midNode = nodes[1];
        const auto& endNode = nodes[2];
        for (const auto& edge : parser.getEdges())
        {
            if (edge["lhs"]["identifier"] == midNode && edge["rhs"]["identifier"] == endNode)
            {
                const auto label = edge["label"];
                if (label["kind"] == "Comparison" &&
                    label["lhs"]["kind"] == "Access" &&
                    label["lhs"]["lhs"]["kind"] == "Access" &&
                    label["lhs"]["lhs"]["lhs"]["kind"] == "Reference")
                {
                    constantMap = label["lhs"]["lhs"]["lhs"]["identifier"].get<std::string>();
                    iteratorIndex = label["lhs"]["lhs"]["rhs"]["identifier"].get<std::string>();
                    const auto sourceType = parser.getSourceType(parser.findTypeOfVariable(constantMap));
                    const auto [minValue, _] = symbolsManager.getValueAssigner().getTypeMinMaxValues(sourceType);
                    if (minValue != 0)
                    {
                        iteratorIndex += " - " + std::to_string(minValue);
                    }
                }
                else
                {
                    throw std::runtime_error("[Iterator] Pragma iterator created for invalid edge: " + label.dump());
                }
            }
        }
        if (constantMap.empty())
        {
            throw std::runtime_error("[Iterator] Couldn't find edge between " + midNode + " and " + endNode);
        }
        const auto iteratorName = constantMap + ITERATOR_SUFFIX.data();
        iteratorNames_.insert(iteratorName);
        nodesAndVariables_.emplace_back(
            NodesAndVariable {
                .nodes = nodes,
                .variable = variable,
                .iteratorName = iteratorName,
                .iteratorIndex = iteratorIndex});
    }
}

// TODO: remove unused "contains" methods
bool IteratorData::contains(const std::shared_ptr<Node>& lhsNode, const std::shared_ptr<Node>& rhsNode) const
{
    for (const auto& nodesAndVariable : nodesAndVariables_)
    {
        const auto& nodes = nodesAndVariable.nodes;
        for (auto node = nodes.begin(); node + 1 != nodes.end(); ++node)
        {
            if (*node == lhsNode->getName() && *(node + 1) == rhsNode->getName())
            {
                return true;
            }
        }
    }
    return false;
}

bool IteratorData::contains(const std::shared_ptr<Node>& node) const
{
    for (const auto& nodesAndVariable : nodesAndVariables_)
    {
        const auto& nodes = nodesAndVariable.nodes;
        if (std::find(nodes.begin(), nodes.end(), node->getName()) != nodes.end())
        {
            return true;
        }
    }
    return false;
}

// TODO: assignmentAnyAction not used
bool IteratorData::contains(
    const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& assignmentAnyAction) const
{
    for (const auto& nodesAndVariable : nodesAndVariables_)
    {
        const auto& nodes = nodesAndVariable.nodes;
        if (nodes.front() == node->getName())
        {
            return true;
        }
    }
    return false;
}

std::string IteratorData::getRangeName(
    const std::shared_ptr<Node>& node, const std::shared_ptr<IAction>& assignmentAnyAction) const
{
    for (const auto& [nodes, variable, iteratorName, iteratorIndex] : nodesAndVariables_)
    {
        if (std::find(nodes.begin(), nodes.end(), node->getName()) != nodes.end())
        {
            return iteratorName + "[" + iteratorIndex + "]";
        }
    }
    throw std::runtime_error("[Iterator] Cannot find pragma iterator for node: " + node->getName());
}

const std::set<std::string> IteratorData::getIteratorNames() const
{
    return iteratorNames_;
}

const std::string IteratorData::getConstantNameForIterator(const std::string& iteratorName) const
{
    return iteratorName.substr(0, iteratorName.size() - ITERATOR_SUFFIX.size());
}
