#include <queue>

#include <common/Common.hpp>
#include <compiler/graphOperations/GenerateGraphsOperator.hpp>

std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> GenerateGraphsOperator::forPatterns(
    ActionType actionType) const
{
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternGraphs;
    std::set<std::pair<std::string, std::string>> patterns;

    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        const auto &action = edge->getActions().front();

        if (action->getType() == actionType)
        {
            patterns.insert(std::make_pair(action->getLeftSide(), action->getRightSide()));
        }
    }

    for (const auto &[from, to] : patterns)
    {
        patternGraphs.push_back(std::make_tuple(from, to, generateGraphForPattern(from, {graph_->getNodeId(to)})));
    }

    return patternGraphs;
}

std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>> GenerateGraphsOperator::forApplyAnyMove()
    const
{
    std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>> graphs;
    std::map<std::string, std::set<int>> edgesWithActionChangePlayerForToNode;
    std::set<int> edgesWithActionChangePlayer;

    for (const auto &v : graph_->getAllEdges())
    {
        const auto &[edge, iid] = v;
        const std::string &fromName = edge->fromName();
        const std::string &toName = edge->toName();

        if (common::isActionAssignmentToPlayer(edge->getActions().back()))
        {
            int edgeId = graph_->getEdgeId(fromName, toName, iid);
            edgesWithActionChangePlayer.insert(edgeId);
            auto &p = edgesWithActionChangePlayerForToNode[toName];
            p.insert(edgeId);
        }
    }

    auto getBannedEdges = [&edgesWithActionChangePlayer](const std::set<int> &goodEdges) {
        std::set<int> res;

        for (int edge : edgesWithActionChangePlayer)
        {
            if (goodEdges.find(edge) == goodEdges.end())
            {
                res.insert(edge);
            }
        }

        return res;
    };

    for (const auto &fromName : getNodesBeforeWhichPlayerChangeToKeeper())
    {
        std::set<int> bannedEdges;
        std::set<int> toNames;
        for (const auto &toName : nodesToPlayerChangeOrEnd(fromName))
        {
            bannedEdges.insert(
                edgesWithActionChangePlayerForToNode[toName].begin(),
                edgesWithActionChangePlayerForToNode[toName].end());
            toNames.insert(graph_->getNodeId(toName));
        }
        std::shared_ptr<Graph> graph = generateGraphForPattern(fromName, toNames, getBannedEdges(bannedEdges));

        if (!graph->empty())
        {
            graphs.push_back({fromName, toNames, graph});
        }
    }

    return graphs;
}

std::vector<std::string> GenerateGraphsOperator::getNodesBeforeWhichPlayerChangeToKeeper() const
{
    std::set<std::string> nodes({std::string(common::BEGIN_WORD)});
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        if (common::isActionAssignmentKeeperToPlayer(edge->getActions().back()))
        {
            nodes.insert(edge->toName());
        }
    }

    return std::vector<std::string>(nodes.begin(), nodes.end());
}

std::shared_ptr<Graph> GenerateGraphsOperator::generateGraphForPattern(
    std::string from, const std::set<int> &to, const std::set<int> &bannedEdges) const
{
    std::shared_ptr<Graph> graph = std::make_shared<Graph>();

    std::set<int> visited;
    std::set<int> nodesInPatternGraph;

    int lastChanged = -1;
    // TODO: This runs in O(N^*E) we should change it to O(E)
    while (true)
    {
        visited.clear();
        generatePathFromNodeToNode(graph_->getNodeId(from), to, visited, nodesInPatternGraph, bannedEdges);
        if (nodesInPatternGraph.size() == lastChanged)
        {
            break;
        }
        lastChanged = nodesInPatternGraph.size();
    }

    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        // We should work on not optimized graph
        assert(edge->getActions().size() == 1);

        if (!to.count(graph_->getNodeId(edge->fromName())) &&
            nodesInPatternGraph.count(graph_->getNodeId(edge->fromName())) &&
            nodesInPatternGraph.count(graph_->getNodeId(edge->toName())) &&
            bannedEdges.find(graph_->getEdgeId(edge->fromName(), edge->toName(), iid)) == bannedEdges.end())
        {
            graph->addEdge(edge);
        }
    }

    return graph;
}

std::vector<std::string> GenerateGraphsOperator::nodesToPlayerChangeOrEnd(const std::string &nodeName) const
{
    std::set<std::string> visited;
    std::string node = nodeName;
    std::vector<std::string> nodes;
    std::queue<std::string> nodesToVisit({node});
    while (!nodesToVisit.empty())
    {
        std::string node = nodesToVisit.front();
        nodesToVisit.pop();
        for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(node))
        {
            if (visited.find(edge->toName()) == visited.end())
            {
                visited.insert(edge->toName());
                if (common::isActionAssignmentToPlayer(edge->getActions().back()) || edge->toName() == common::END_WORD)
                {
                    nodes.push_back(edge->toName());
                    continue;
                }
                nodesToVisit.push(edge->toName());
            }
        }
    }

    return nodes;
}

bool GenerateGraphsOperator::generatePathFromNodeToNode(
    int node,
    const std::set<int> &finalNodes,
    std::set<int> &visited,
    std::set<int> &nodesInPatternGraph,
    const std::set<int> &bannedEdges) const
{
    if (finalNodes.count(node))
    {
        nodesInPatternGraph.insert(node);
        return true;
    }

    if (visited.count(node))
    {
        return nodesInPatternGraph.find(node) != nodesInPatternGraph.end();
    }

    visited.insert(node);
    bool havePathToFinalNode = false;

    for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(node))
    {
        if (bannedEdges.find(graph_->getEdgeId(edge->fromName(), edge->toName(), iid)) != bannedEdges.end())
        {
            continue;
        }

        if (generatePathFromNodeToNode(
                graph_->getNodeId(edge->toName()), finalNodes, visited, nodesInPatternGraph, bannedEdges))
        {
            havePathToFinalNode = true;
        }
    }

    if (havePathToFinalNode)
    {
        nodesInPatternGraph.insert(node);
    }

    return havePathToFinalNode;
}

GenerateGraphsOperator::GenerateGraphsOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}
