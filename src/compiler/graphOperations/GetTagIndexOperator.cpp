#include <compiler/graphOperations/GetTagIndexOperator.hpp>

void GetTagIndexOperator::init(const Parser& parser)
{
    int maxSize = 0;
    for (const auto& pragma : parser.getPragmas("TagIndex"))
    {
        int position = pragma["index"];
        maxSize = std::max(maxSize, position);
        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            const auto node = Node(nodeStruct["parts"]);
            nodeNameToTagPosition_[node.getName()] = position;
        }
    }

    std::set<std::string> nodesWithTagIndexMax;
    for (const auto& pragma : parser.getPragmas("TagIndexMax"))
    {
        maxSize = std::max(maxSize, static_cast<int>(pragma["index"]));

        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            const auto node = Node(nodeStruct["parts"]);
            nodesWithTagIndexMax.insert(node.getName());
        }
    }

    containerSize_ = maxSize + 1;
    allTagsInSamePosition_ = true;
    bool allNodesHaveTagIndexMax = true;
    for (auto& [edge, iid] : graph_->getAllEdges())
    {
        std::vector<std::shared_ptr<Node>> fromNodes = {edge->getLeftNode()};
        fromNodes.insert(fromNodes.end(), edge->getInnerNodes().begin(), edge->getInnerNodes().end());
        auto fromNodeIt = fromNodes.begin();
        assert(fromNodes.size() == edge->getActions().size());

        for (const auto& action : edge->getActions())
        {
            if (action->getType() == ActionType::Tag)
            {
                if (!nodesWithTagIndexMax.count((*fromNodeIt)->getName()))
                {
                    allNodesHaveTagIndexMax = false;
                }

                if (!nodeNameToTagPosition_.count((*fromNodeIt)->getName()))
                {
                    allTagsInSamePosition_ = false;
                }
            }
            fromNodeIt++;
        }
    }

    if (!allNodesHaveTagIndexMax && !allTagsInSamePosition_)
    {
        containerSize_ = -1;
    }
}

bool GetTagIndexOperator::allTagsInSamePosition() const
{
    return allTagsInSamePosition_;
}
int GetTagIndexOperator::containerSize() const
{
    return containerSize_;
}

int GetTagIndexOperator::getTagPositionForNode(const std::string& nodeName) const
{
    if (!nodeNameToTagPosition_.count(nodeName))
    {
        return -1;
    }
    return nodeNameToTagPosition_.at(nodeName);
}

GetTagIndexOperator::GetTagIndexOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}