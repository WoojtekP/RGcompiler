#include <compiler/graphOperations/GetOptimizedGraphOperator.hpp>

void GetOptimizedGraphOperator::traverse(int node, std::vector<int> &path, std::map<int, bool> &visited) const
{
    const auto &action = graph_->getEdge(graph_->getNode(path.back())->getName(), graph_->getNode(node)->getName(), 0)
                             ->getActions()
                             .back();
    path.push_back(node);
    if (graph_->getOutgoingEdgesFrom(node).size() != 1 ||
        numberOfIncomingEdges_.at(graph_->getNodeId(graph_->getNode(node)->getName())) != 1)
    {
        return;
    }

    // There is only one incoming edge for "node" therefor iid should be equal to 0
    if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
    {
        return;
    }

    visited.at(node) = true;
    int newNode = graph_->getNodeId(graph_->getOutgoingEdgesFrom(node).front().first->toName());
    traverse(newNode, path, visited);
}

void GetOptimizedGraphOperator::traverseCycle(int node, std::vector<int> &path, std::map<int, bool> &visited) const
{
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
        if (numberOfIncomingEdges_.at(graph_->getNodeId(nodeName)) != 1 ||
            graph_->getOutgoingEdgesFrom(nodeName).size() != 1)
        {
            visited.at(nodeId) = true;
            for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(nodeName))
            {
                int nextNodeId = graph_->getNodeId(edge->toName());
                std::vector<int> path {nodeId};

                traverse(nextNodeId, path, visited);

                paths.push_back(path);

                while (numberOfIncomingEdges_.at(graph_->getNodeId(graph_->getNode(path.back())->getName())) == 1 &&
                       graph_->getOutgoingEdgesFrom(path.back()).size() == 1)
                {
                    visited.at(path.back()) = true;
                    int lastNode = path.back();
                    path.clear();
                    path.push_back(lastNode);
                    int newNode = graph_->getNodeId(graph_->getOutgoingEdgesFrom(lastNode).front().first->toName());
                    traverse(newNode, path, visited);
                    paths.push_back(path);
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
}