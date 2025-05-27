#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

class GetTagIndexOperator : public BaseOperator
{
    int containerSize_ = -1;
    int maxDefinedIndex_ = -1;
    bool allTagsInSamePosition_ = false;
    std::map<std::string, int> nodeNameToTagPosition_;

    std::shared_ptr<Node> fillPositions(
        std::shared_ptr<Node> node,
        const std::vector<std::string> tags,
        std::vector<int> &positions,
        std::set<int> &visited);

public:
    GetTagIndexOperator(const std::shared_ptr<Graph> &graph);
    void init(const Parser &parser);
    bool allTagsInSamePosition() const;
    int containerSize() const;
    int maxDefinedIndex() const;
    int getTagPositionForNode(const std::string &nodeName) const;
    std::pair<std::shared_ptr<Node>, int> getPositions(std::shared_ptr<Node>, const std::string &tag);
};
