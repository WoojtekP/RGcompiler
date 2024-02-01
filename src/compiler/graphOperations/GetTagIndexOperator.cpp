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
            std::string nodeName = nodeStruct["parts"][0]["identifier"];
            nodeNameToTagPosition_[nodeName] = position;
        }
    }

    std::set<std::string> nodesWithTagIndexMax;
    for (const auto& pragma : parser.getPragmas("TagIndexMax"))
    {
        maxSize = std::max(maxSize, static_cast<int>(pragma["index"]));

        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            std::string nodeName = nodeStruct["parts"][0]["identifier"];
            nodesWithTagIndexMax.insert(nodeName);
        }
    }

    containerSize_ = maxSize + 1;
    allTagsInSamePosition_ = true;
    bool allNodesHaveTagIndexMax = true;
    for (auto& [edge, iid] : graph_->getAllEdges())
    {
        for (const auto& action : edge->getActions())
        {
            if (action->getType() == ActionType::Tag)
            {
                if (!nodesWithTagIndexMax.count(edge->fromName()))
                {
                    allNodesHaveTagIndexMax = false;
                }

                if (!nodeNameToTagPosition_.count(edge->fromName()))
                {
                    allTagsInSamePosition_ = false;
                }
            }
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

int GetTagIndexOperator::getTagPositionForNode(const std::string& node) const
{
    if (!nodeNameToTagPosition_.count(node))
    {
        return -1;
    }
    return nodeNameToTagPosition_.at(node);
}

GetTagIndexOperator::GetTagIndexOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}