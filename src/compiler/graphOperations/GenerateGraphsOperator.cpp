#include <queue>

#include <compiler/graphOperations/GenerateGraphsOperator.hpp>
std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> GenerateGraphsOperator::forPatterns(
    ActionType actionType) const
{
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternGraphs;

    std::set<std::pair<std::string, std::string>> patterns;

    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        const auto &action = edge->getActions().front();

        if (action->getType() == actionType &&
            patterns.find(std::make_pair(action->getLeftSide(), action->getRightSide())) == patterns.end())
        {
            patterns.insert(std::make_pair(action->getLeftSide(), action->getRightSide()));
        }
    }

    for (const auto &[from, to] : patterns)
    {
        patternGraphs.push_back(std::make_tuple(from, to, generateGraphForPattern(from, to)));
    }

    return patternGraphs;
}

std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> GenerateGraphsOperator::forApplyAnyMove()
    const
{
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> graphs;
    std::map<std::string, std::set<int>> edgesWithActionChangePlayerForToNode;
    std::set<int> edgesWithActionChangePlayer;

    for (const auto &v : graph_->getAllEdges())
    {
        const auto &[edge, iid] = v;
        const auto &fromName = edge->fromName();
        const auto &toName = edge->toName();
        const auto &action = edge->getActions().back();

        if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
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
        for (const auto &toName : nodesToPlayerChangeOrEnd(fromName))
        {
            std::set<int> bannedEdges = getBannedEdges(edgesWithActionChangePlayerForToNode[toName]);
            std::shared_ptr<Graph> graph = generateGraphForPattern(fromName, toName, bannedEdges);

            if (!graph->empty())
            {
                graphs.push_back({fromName, toName, graph});
            }
        }
    }

    return graphs;
}

std::vector<std::string> GenerateGraphsOperator::getNodesBeforeWhichPlayerChangeToKeeper() const
{
    std::set<std::string> nodes({"begin"});
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        const auto &action = edge->getActions().back();
        // TODO: We need better way to check if keeper changed
        if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player" &&
            (action->getRightSide() == "static_cast<PlayerOrKeeper>(keeper)" || action->getRightSide() == "keeper"))
        {
            nodes.insert(edge->toName());
        }
    }

    return std::vector<std::string>(nodes.begin(), nodes.end());
}

std::shared_ptr<Graph> GenerateGraphsOperator::generateGraphForPattern(
    std::string from, std::string to, const std::set<int> &bannedEdges) const
{
    std::shared_ptr<Graph> graph = std::make_shared<Graph>();

    std::vector<std::shared_ptr<Edge>> edges;

    std::vector<bool> visited(graph_->getAllEdges().size(), false);
    std::vector<bool> nodesOnPathToFinalNode(graph_->getNumberOfNodes(), false);
    nodesOnPathToFinalNode[graph_->getNodeId(to)] = true;

    // generatePathFromNodeToNode(from, to, edges, visited, nodesOnPathToFinalNode, bannedEdges);

    // Temporary fix for cyclic graphs
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        graph->addEdge(edge);
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
                const auto &action = edge->getActions().back();
                if ((action->getType() == ActionType::Assignment && action->getLeftSide() == "player") ||
                    edge->toName() == "end")
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
    std::string node,
    std::string finalNode,
    std::vector<std::shared_ptr<Edge>> &edges,
    std::vector<bool> &visited,
    std::vector<bool> &onPathToFinalNode,
    const std::set<int> &bannedEdges) const
{
    if (node == finalNode)
    {
        return true;
    }

    bool havePathToFinalNode = false;

    for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(node))
    {
        int edgeId = graph_->getEdgeId(edge->fromName(), edge->toName(), iid);
        int edgeShiftedId = edgeId - graph_->getNumberOfNodes();

        if (bannedEdges.find(edgeId) != bannedEdges.end())
        {
            continue;
        }

        if (!visited[edgeShiftedId])
        {
            visited[edgeShiftedId] = true;
            if (generatePathFromNodeToNode(edge->toName(), finalNode, edges, visited, onPathToFinalNode, bannedEdges))
            {
                havePathToFinalNode = true;
                edges.push_back(edge);
            }
        }
        else if (onPathToFinalNode[graph_->getNodeId(edge->toName())])
        {
            havePathToFinalNode = true;
        }
    }

    if (havePathToFinalNode)
    {
        onPathToFinalNode[graph_->getNodeId(node)] = true;
    }

    return havePathToFinalNode;
}

GenerateGraphsOperator::GenerateGraphsOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}