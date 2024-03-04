#include <compiler/graphOperations/GetOptimizedGraphOperator.hpp>

void GetOptimizedGraphOperator::traverse(
    const std::shared_ptr<Edge> &edge, std::vector<int> &path, std::map<int, bool> &visited) const
{
    int nextNodeId = graph_->getNodeId(edge->getRightNode()->getName());
    path.push_back(nextNodeId);

    // If this node have more than one outgoing or incoming edges then it can't form simple path
    if (graph_->getOutgoingEdgesFrom(nextNodeId).size() != 1 || numberOfIncomingEdges_.at(nextNodeId) != 1)
    {
        return;
    }

    // According to rules of rg language assignment to player end move therfore end path
    const auto &action = edge->getActions().back();
    if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
    {
        return;
    }

    visited.at(nextNodeId) = true;
    traverse(graph_->getOutgoingEdgesFrom(nextNodeId).back().first, path, visited);
}

void GetOptimizedGraphOperator::traverseCycle(int node, std::vector<int> &path, std::map<int, bool> &visited) const
{
    assert(graph_->getOutgoingEdgesFrom(node).size() == 1);
    path.push_back(node);

    if (visited[node])
    {
        return;
    }

    visited.at(node) = true;
    int newNode = graph_->getNodeId(graph_->getOutgoingEdgesFrom(node).front().first->toName());
    traverseCycle(newNode, path, visited);
}

void GetOptimizedGraphOperator::initializeNumberOfIncomingEdges()
{
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        numberOfIncomingEdges_[graph_->getNodeId(edge->toName())]++;
    }

    // Insert 0 for nodes without incoming edges
    for (const auto &node : graph_->getAllNodes())
    {
        numberOfIncomingEdges_.insert({graph_->getNodeId(node->getName()), 0});
    }
}

void GetOptimizedGraphOperator::initializeNodesUsedInReachabilityPattern()
{
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        if (edge->getActions().front()->getType() == ActionType::Reachability)
        {
            nodesUsedInReachability_.insert(graph_->getNodeId(edge->fromName()));
            nodesUsedInReachability_.insert(graph_->getNodeId(edge->toName()));
        }
    }
}

std::shared_ptr<Graph> GetOptimizedGraphOperator::getGraphWithOptimizedPaths()
{
    if (optimizedGraph_)
    {
        return optimizedGraph_;
    }

    std::vector<std::vector<int>> paths;
    std::map<int, bool> visited;

    for (const auto &node : graph_->getAllNodes())
    {
        visited[graph_->getNodeId(node->getName())] = false;
    }

    for (const auto &node : graph_->getAllNodes())
    {
        std::string nodeName = node->getName();
        int nodeId = graph_->getNodeId(nodeName);
        if (numberOfIncomingEdges_.at(nodeId) != 1 || graph_->getOutgoingEdgesFrom(nodeId).size() != 1)
        {
            visited.at(nodeId) = true;
            for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(nodeId))
            {
                std::vector<int> path {nodeId};
                traverse(edge, path, visited);

                paths.push_back(path);

                int lastNode = path.back();
                while (numberOfIncomingEdges_.at(lastNode) == 1 && graph_->getOutgoingEdgesFrom(lastNode).size() == 1)
                {
                    visited.at(lastNode) = true;
                    path.clear();
                    path.push_back(lastNode);
                    traverse(graph_->getOutgoingEdgesFrom(lastNode).back().first, path, visited);
                    paths.push_back(path);
                    lastNode = path.back();
                }
            }
        }
    }

    for (const auto &node : graph_->getAllNodes())
    {
        int nodeId = graph_->getNodeId(node->getName());

        if (!visited.at(nodeId))
        {
            std::vector<int> path;

            traverseCycle(nodeId, path, visited);

            paths.push_back(path);
        }
    }

    std::shared_ptr<Graph> newGraph = std::make_shared<Graph>();
    std::map<std::pair<std::string, std::string>, int> countRepetitions;
    for (const auto &path : paths)
    {
        assert(!path.empty());

        int firstNode = path.front();
        int lastNode = path.back();

        std::vector<std::shared_ptr<IAction>> actions;

        for (int i = 0; i < path.size() - 1; i++)
        {
            std::string fromName = graph_->getNode(path[i])->getName();
            std::string toName = graph_->getNode(path[i + 1])->getName();
            const auto &edge = graph_->getEdge(fromName, toName, countRepetitions[{fromName, toName}]++);

            for (const auto &action : edge->getActions())
            {
                actions.push_back(action);
            }
        }

        std::vector<std::shared_ptr<Node>> innerNodes;

        for (int i = 1; i < path.size() - 1; i++)
        {
            innerNodes.push_back(graph_->getNode(path[i]));
        }

        newGraph->addEdge(std::move(
            std::make_shared<Edge>(graph_->getNode(firstNode), graph_->getNode(lastNode), actions, innerNodes)));
    }

    newGraph->initialize();

    optimizedGraph_ = newGraph;
    return newGraph;
}

GetOptimizedGraphOperator::GetOptimizedGraphOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
, optimizedGraph_(nullptr)
{
    initializeNumberOfIncomingEdges();
    initializeNodesUsedInReachabilityPattern();
}