#include <compiler/graphOperations/PragmaDisjointOperator.hpp>

void PragmaDisjointOperator::init(const Parser& parser)
{
    for (const auto& pragma : parser.getPragmas("Disjoint"))
    {
        std::string mainNodeName = pragma["edgeName"]["identifier"];
        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            std::string nodeName = nodeStruct["identifier"];
            nodeNameToOrderOfDisjointNodes_[mainNodeName].push_back(nodeName);
        }
    }

    for (const auto& pragma : parser.getPragmas("DisjointExhaustive"))
    {
        std::string mainNodeName = pragma["edgeName"]["identifier"];
        exhaustive_.insert(mainNodeName);

        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            std::string nodeName = nodeStruct["identifier"];
            nodeNameToOrderOfDisjointNodes_[mainNodeName].push_back(nodeName);
        }
    }
}

bool PragmaDisjointOperator::isDisjoint(const std::string& node) const
{
    return nodeNameToOrderOfDisjointNodes_.count(node);
}

bool PragmaDisjointOperator::isExhaustive(const std::string& node) const
{
    return exhaustive_.count(node);
}

const std::vector<std::string>& PragmaDisjointOperator::getNodeNames(const std::string& nodeName) const
{
    return nodeNameToOrderOfDisjointNodes_.at(nodeName);
}

PragmaDisjointOperator::PragmaDisjointOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}
