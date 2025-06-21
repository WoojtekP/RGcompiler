#include <queue>

#include <common/Common.hpp>
#include <compiler/graphOperations/GenerateGraphsOperator.hpp>

namespace
{
int fixParent(int nodeId, std::map<int, int> &nodeIdToOldestParent)
{
    if (nodeIdToOldestParent[nodeId] == nodeId)
    {
        return nodeId;
    }
    nodeIdToOldestParent[nodeId] = fixParent(nodeIdToOldestParent[nodeId], nodeIdToOldestParent);
    return nodeIdToOldestParent[nodeId];
}
}  // namespace

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

    std::set<int> nodesInPatternGraph;
    std::map<int, int> nodeIdToOldestParent;
    std::map<int, int> visitTime;
    int visitedTimestampId = 0;
    generatePathFromNodeToNode(
        graph_->getNodeId(from),
        to,
        nodeIdToOldestParent,
        visitTime,
        nodesInPatternGraph,
        bannedEdges,
        visitedTimestampId);

    for (auto [nodeId, parentNodeId] : nodeIdToOldestParent)
    {
        parentNodeId = fixParent(parentNodeId, nodeIdToOldestParent);
        if (nodesInPatternGraph.count(parentNodeId))
        {
            nodesInPatternGraph.insert(nodeId);
        }
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
    std::map<int, int> &nodeIdToOldestParent,
    std::map<int, int> &visitTime,
    std::set<int> &nodesInPatternGraph,
    const std::set<int> &bannedEdges,
    int &timestampId) const
{
    if (finalNodes.count(node))
    {
        if (!visitTime.count(node))
        {
            visitTime[node] = timestampId++;
            nodeIdToOldestParent[node] = node;
        }
        nodesInPatternGraph.insert(node);
        return true;
    }

    if (visitTime.count(node))
    {
        return false;
    }

    visitTime[node] = timestampId++;
    nodeIdToOldestParent[node] = node;

    bool havePathToFinalNode = false;

    for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(node))
    {
        if (bannedEdges.find(graph_->getEdgeId(edge->fromName(), edge->toName(), iid)) != bannedEdges.end())
        {
            continue;
        }
        int newNodeId = graph_->getNodeId(edge->toName());

        if (generatePathFromNodeToNode(
                newNodeId, finalNodes, nodeIdToOldestParent, visitTime, nodesInPatternGraph, bannedEdges, timestampId))
        {
            havePathToFinalNode = true;
        }

        if (visitTime[nodeIdToOldestParent[newNodeId]] < visitTime[nodeIdToOldestParent[node]])
        {
            nodeIdToOldestParent[node] = nodeIdToOldestParent[newNodeId];
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

std::shared_ptr<Graph> GenerateGraphsOperator::forMainGraph() const
{
    int nodeId = graph_->getNodeId(std::string(common::END_WORD));
    assert(nodeId != -1);
    return generateGraphForPattern(std::string(common::BEGIN_WORD), {nodeId});
}
