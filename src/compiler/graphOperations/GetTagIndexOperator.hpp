#pragma once
#include <set>
#include <string>

#include <compiler/graphOperations/BaseOperator.hpp>

class GetTagIndexOperator : public BaseOperator
{
    int containerSize_ = -1;
    bool allTagsInSamePosition_ = false;
    std::map<std::string, int> nodeNameToTagPosition_;

public:
    GetTagIndexOperator(const std::shared_ptr<Graph> &graph);
    void init(const Parser &parser);
    bool allTagsInSamePosition() const;
    int containerSize() const;
    int getTagPositionForNode(const std::string &node) const;
};