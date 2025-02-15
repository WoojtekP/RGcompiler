#include "RepeatFlat.hpp"


void RepeatFlatData::parse(const Parser& parser)
{
    const auto isVariableOfFunctionType = [&parser](const auto& variableName) {
        const auto& variableType = parser.findTypeOfVariable(variableName);
        if (variableType["kind"] == "TypeReference")
        {
            const auto& typeDefinition = parser.findTypeByIdentifier(variableType["identifier"]);
            return typeDefinition["type"]["kind"] == "Arrow";
        }
        return variableType["kind"] == "Arrow";
    };

    for (const auto& pragma : parser.getPragmas("Repeat"))
    {
        if (std::any_of(pragma["identifiers"].begin(), pragma["identifiers"].end(), isVariableOfFunctionType))
        {
            continue;
        }
        for (const auto& edge : pragma["edgeNames"])
        {
            const auto& nodeName = edge["identifier"].get<std::string>();
            repeatNodes_.insert(nodeName);
            auto& identifiers = stateToIdentifiers_[nodeName];
            for (const auto& variableName : pragma["identifiers"])
            {
                identifiers.push_back(variableName);
            }
        }
    }
}

void RepeatFlatData::initializeDataForGraphs(
    const std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& graphs,
    const std::shared_ptr<Graph>& mainGraph,
    const int patternId)
{
    for (const auto& [from, to, graph] : graphs)
    {
        std::set<int> repeatNodesForGraph;
        for (const auto& node : graph->getAllNodes())
        {
            if (repeatNodes_.count(node->getName()))
            {
                repeatNodesForGraph.insert(mainGraph->getNodeId(node->getName()));
            }
        }
        if (!repeatNodesForGraph.empty())
        {
            typeOfGraphToStatesToClear_.emplace(std::make_tuple(from, to, patternId), repeatNodesForGraph);
        }
    }
}


const StateToIdsMap& RepeatFlatData::getStateToIdentifiersMap() const
{
    return stateToIdentifiers_;
}

const NodesSet& RepeatFlatData::getRepeatNodes() const
{
    return repeatNodes_;
}

const TypeOfGraphToStatesMap& RepeatFlatData::getTypeOfGraphToStatesMap() const
{
    return typeOfGraphToStatesToClear_;
}
