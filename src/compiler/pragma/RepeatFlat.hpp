#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <parser/Parser.hpp>
#include <graph/Graph.hpp>


using StateToIdsMap = std::map<std::string, std::vector<std::string>>;
using NodesSet = std::set<std::string>;
using TypeOfGraphToStatesMap = std::map<std::tuple<std::string, std::string, int>, std::set<int>>;

class RepeatFlatData
{
public:
    void parse(const Parser& parser);
    void initializeDataForGraphs(
        const std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& graphs,
        const std::shared_ptr<Graph>& mainGraph,
        const int patternId);

    const StateToIdsMap& getStateToIdentifiersMap() const;
    const NodesSet& getRepeatNodes() const;
    const TypeOfGraphToStatesMap& getTypeOfGraphToStatesMap() const;

private:
    StateToIdsMap stateToIdentifiers_;
    NodesSet repeatNodes_;
    TypeOfGraphToStatesMap typeOfGraphToStatesToClear_;
};
