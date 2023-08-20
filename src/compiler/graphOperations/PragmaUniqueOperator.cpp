#include <compiler/graphOperations/PragmaUniqueOperator.hpp>

void PragmaUniqueOperator::uniqueDfs(int node, std::set<int> &result)
{
    result.insert(node);

    for (const auto &[edge, iid] : graph_->getOutgoingEdgesFrom(graph_->getNode(node)->getName()))
    {
        bool newUniqueTag = false;
        for (const auto &action : edge->getActions())
        {
            if (action->getType() == ActionType::Tag)
            {
                newUniqueTag = true;
                continue;
            }
        }

        if (!newUniqueTag)
        {
            uniqueDfs(graph_->getNodeId(edge->toName()), result);
        }
    }
}

void PragmaUniqueOperator::findUniqeNodesOnPaths(const std::vector<int> &startNodes)
{
    for (int node : startNodes)
    {
        uniqueDfs(node, nodesOnUniquePaths_);
    }
}

bool PragmaUniqueOperator::isOnUniquePath(int node) const
{
    return nodesOnUniquePaths_.find(node) != nodesOnUniquePaths_.end();
}

void PragmaUniqueOperator::init(const Parser &parser)
{
    std::vector<int> uniqueNodes;
    nodesOnUniquePaths_.clear();

    for (const auto &pragma : parser.getPragmas("Unique"))
    {
        std::string nodeName = pragma["edgeName"]["parts"][0]["identifier"];
        auto node = graph_->getNodeIdOptional(nodeName);
        if (node)
        {
            uniqueNodes.push_back(*node);
        }
    }

    findUniqeNodesOnPaths(uniqueNodes);
}

std::set<std::string> PragmaUniqueOperator::getNodes() const
{
    std::set<std::string> res;
    for (int id : nodesOnUniquePaths_)
    {
        res.insert(graph_->getNode(id)->getName());
    }
    return res;
}

void PragmaUniqueOperator::init(const std::set<std::string> &nodes)
{
    nodesOnUniquePaths_.clear();
    for (const auto &name : nodes)
    {
        nodesOnUniquePaths_.insert(graph_->getNodeId(name));
    }
}

PragmaUniqueOperator::PragmaUniqueOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{}