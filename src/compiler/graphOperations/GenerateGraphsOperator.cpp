#include <queue>
#include <stack>

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

void GenerateGraphsOperator::prepareOrderForSCC(
    int nodeId, std::stack<int> &orderOfNodes, std::shared_ptr<Graph> revGraph, std::set<int> &visited) const
{
    visited.insert(nodeId);

    for (const auto &[edge, iid] : revGraph->getOutgoingEdgesFrom(nodeId))
    {
        int newNodeId = graph_->getNodeId(edge->getRightNode()->getName());
        if (visited.insert(newNodeId).second)
        {
            prepareOrderForSCC(newNodeId, orderOfNodes, revGraph, visited);
        }
    }

    orderOfNodes.push(nodeId);
}

void GenerateGraphsOperator::assignToSCC(
    int nodeId, int sccId, std::shared_ptr<Graph> revGraph, std::map<int, int> &nodeIdToSccId) const
{
    nodeIdToSccId[nodeId] = sccId;

    for (const auto &[edge, iid] : revGraph->getOutgoingEdgesFrom(nodeId))
    {
        int newNodeId = graph_->getNodeId(edge->getRightNode()->getName());
        if (!nodeIdToSccId.count(newNodeId))
        {
            assignToSCC(newNodeId, sccId, revGraph, nodeIdToSccId);
        }
    }
}

bool GenerateGraphsOperator::getSccIdsOnPathToFinalNodes(
    int nodeId,
    const std::set<int> &finalNodeSccIds,
    const std::map<int, std::set<int>> &connectionsInSCCGraph,
    std::set<int> &visited,
    std::set<int> &sccIdsOnPathToFinalNodes) const
{
    visited.insert(nodeId);
    if (finalNodeSccIds.count(nodeId))
    {
        sccIdsOnPathToFinalNodes.insert(nodeId);
        return true;
    }
    bool havePathToFinalNode = false;
    if (!connectionsInSCCGraph.count(nodeId))
    {
        return false;
    }
    for (int newNodeId : connectionsInSCCGraph.at(nodeId))
    {
        // it should be DAG
        assert(!visited.count(newNodeId));
        if (getSccIdsOnPathToFinalNodes(
                newNodeId, finalNodeSccIds, connectionsInSCCGraph, visited, sccIdsOnPathToFinalNodes))
        {
            havePathToFinalNode = true;
        }
    }
    if (havePathToFinalNode)
    {
        sccIdsOnPathToFinalNodes.insert(nodeId);
    }
    return havePathToFinalNode;
}

std::shared_ptr<Graph> GenerateGraphsOperator::generateGraphForPattern(
    std::string from, const std::set<int> &to, const std::set<int> &bannedEdges) const
{
    std::shared_ptr<Graph> graph = std::make_shared<Graph>();

    // For SCC
    std::shared_ptr<Graph> revGraph = std::make_shared<Graph>();
    std::stack<int> orderOfNodes;
    std::set<int> visited;
    int sccId = 0;
    std::map<int, int> nodeIdToSccId;
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        // We should work on not optimized graph
        assert(edge->getActions().size() == 1);
        revGraph->addEdge(std::make_shared<Edge>(edge->getRightNode(), edge->getLeftNode(), edge->getActions()));
    }
    revGraph->initialize();

    for (auto node : revGraph->getAllNodes())
    {
        int nodeId = graph_->getNodeId(node->getName());
        if (!visited.count(nodeId))
        {
            prepareOrderForSCC(nodeId, orderOfNodes, revGraph, visited);
        }
    }

    while (!orderOfNodes.empty())
    {
        int nodeId = orderOfNodes.top();
        if (!nodeIdToSccId.count(nodeId))
        {
            assignToSCC(nodeId, sccId, revGraph, nodeIdToSccId);
            sccId++;
        }
        orderOfNodes.pop();
    }

    int startNodeSCCId = nodeIdToSccId.at(graph_->getNodeId(from));
    std::set<int> finalNodeSccIds;
    for (int x : to)
    {
        finalNodeSccIds.insert(nodeIdToSccId.at(x));
    }

    std::map<int, std::set<int>> connectionsInSCCGraph;
    for (const auto &[edge, iid] : revGraph->getAllEdges())
    {
        // We should work on not optimized graph
        assert(edge->getActions().size() == 1);
        int leftNodeId = graph_->getNodeId(edge->getLeftNode()->getName());
        int rightNodeId = graph_->getNodeId(edge->getRightNode()->getName());
        int leftSccId = nodeIdToSccId.at(leftNodeId);
        int rightSccId = nodeIdToSccId.at(rightNodeId);
        if (leftSccId != rightSccId)
        {
            connectionsInSCCGraph[leftSccId].insert(rightSccId);
        }
    }

    std::set<int> sccIdsOnPathToFinalNodes;
    visited.clear();
    getSccIdsOnPathToFinalNodes(
        startNodeSCCId, finalNodeSccIds, connectionsInSCCGraph, visited, sccIdsOnPathToFinalNodes);
    std::set<int> nodesInPatternGraph;
    for (auto [nodeId, sccId] : nodeIdToSccId)
    {
        if (sccIdsOnPathToFinalNodes.count(sccId))
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

GenerateGraphsOperator::GenerateGraphsOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}

std::shared_ptr<Graph> GenerateGraphsOperator::forMainGraph() const
{
    int nodeId = graph_->getNodeId(std::string(common::END_WORD));
    assert(nodeId != -1);
    return generateGraphForPattern(std::string(common::BEGIN_WORD), {nodeId});
}
