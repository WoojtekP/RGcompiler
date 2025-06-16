#include "GetTagIndexOperator.hpp"

#include <optional>
#include <set>

#include <parser/Parser.hpp>

namespace
{
std::optional<std::string> getTagType(const std::string& tag)
{
    assert(tag.size());
    std::string tagTmp = tag.substr(1, tag.size());
    auto pos = tagTmp.find(":");

    if (pos != std::string::npos)
    {
        std::string res = tagTmp.substr(pos + 2);
        res.pop_back();
        return res;
    }
    return {};
}

}  // namespace

std::shared_ptr<Node> GetTagIndexOperator::fillPositions(
    std::shared_ptr<Node> node,
    const std::vector<std::string> tags,
    std::vector<int>& positions,
    std::set<int>& visited)
{
    if (tags.size() == positions.size())
    {
        return node;
    }
    if (!visited.insert(graph_->getNodeId(node->getName())).second)
    {
        return nullptr;
    }

    bool tagVisited = false;
    bool rightTagFound = false;
    std::shared_ptr<Node> lastNode;
    for (const auto& [edge, iid] : graph_->getOutgoingEdgesFrom(node->getName()))
    {
        for (const auto& action : edge->getActions())
        {
            if (action->getType() == ActionType::Tag)
            {
                if (action->getLeftSide() == tags[positions.size()])
                {
                    positions.push_back(getTagPositionForNode(node->getName()));
                    rightTagFound = true;
                }
                tagVisited = true;
            }
            else if (action->getType() == ActionType::TagVariable)
            {
                auto tagType = getTagType(tags[positions.size()]);
                if (tagType && tagType == action->getRightSide())
                {
                    positions.push_back(getTagPositionForNode(node->getName()));
                    rightTagFound = true;
                }
                tagVisited = true;
            }
        }
        if (!tagVisited || rightTagFound)
        {
            lastNode = fillPositions(edge->getRightNode(), tags, positions, visited);
            if (lastNode)
            {
                return lastNode;
            }
        }
    }
    return lastNode;
}

void GetTagIndexOperator::init(const Parser& parser)
{
    int maxSize = 0;
    for (const auto& pragma : parser.getPragmas("TagIndex"))
    {
        int position = pragma["index"];
        maxSize = std::max(maxSize, position);
        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            const auto node = Node(nodeStruct);
            nodeNameToTagPosition_[node.getName()] = position;
        }
    }

    std::set<std::string> nodesWithTagIndexMax;
    for (const auto& pragma : parser.getPragmas("TagIndexMax"))
    {
        maxSize = std::max(maxSize, static_cast<int>(pragma["index"]));

        for (const auto& nodeStruct : pragma["edgeNames"])
        {
            const auto node = Node(nodeStruct);
            nodesWithTagIndexMax.insert(node.getName());
        }
    }

    containerSize_ = maxSize + 1;
    maxDefinedIndex_ = maxSize;
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
            if (action->getType() == ActionType::Tag || action->getType() == ActionType::TagVariable)
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

int GetTagIndexOperator::maxDefinedIndex() const
{
    return maxDefinedIndex_;
}

int GetTagIndexOperator::getTagPositionForNode(const std::string& nodeName) const
{
    if (!nodeNameToTagPosition_.count(nodeName))
    {
        return -1;
    }
    return nodeNameToTagPosition_.at(nodeName);
}

std::pair<std::shared_ptr<Node>, int> GetTagIndexOperator::getPositions(
    std::shared_ptr<Node> node, const std::string& tag)
{
    std::vector<int> positions;
    std::set<int> visited;
    std::shared_ptr<Node> lastNode = fillPositions(node, {tag}, positions, visited);
    return {lastNode, positions[0]};
}

GetTagIndexOperator::GetTagIndexOperator(const std::shared_ptr<Graph>& graph)
: BaseOperator(graph)
{}
