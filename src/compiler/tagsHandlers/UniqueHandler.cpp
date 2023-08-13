#include "UniqueHandler.hpp"

UniqueHandler::UniqueHandler(const std::shared_ptr<Graph> &graph, const Parser &parser)
: graph_(graph)
, parser_(parser)
{
    init();
}

bool UniqueHandler::isOnUniquePath(int node)
{
    return nodesOnUniquePaths_.find(node) != nodesOnUniquePaths_.end();
}

void UniqueHandler::init()
{
    std::vector<int> uniqueNodes;

    for (const auto &pragma : parser_.getPragmas("Unique"))
    {
        std::string nodeName = pragma["edgeName"]["parts"][0]["identifier"];
        int node = graph_->getNodeId(nodeName);
        if (node != -1)
        {
            uniqueNodes.push_back(node);
        }
        else
        {
            std::cout << "Bad node: " << nodeName << "\n";
        }
    }
    graph_->findUniqeNodesOnPaths(nodesOnUniquePaths_, uniqueNodes);
}