#pragma once

#include <compiler/graphOperations/BaseOperator.hpp>

class GetEdgeOperator : public BaseOperator
{
    using ReturnType = std::set<std::pair<std::shared_ptr<Edge>, int>>;
    ReturnType getEdges(const std::function<bool(const std::shared_ptr<IAction> &)> &filter) const;
    std::vector<std::tuple<std::string, std::string, int>> edgeNames_;

public:
    GetEdgeOperator(const std::shared_ptr<Graph> &graph);
    ReturnType getEdgesWithActionChangePlayer() const;
    ReturnType getEdgesWithActionTag() const;
    std::vector<std::tuple<std::string, std::string, int>> getEdgeNames();
    std::vector<std::tuple<std::string, std::string, int>> getUnambiguousPathFromNode(
        const std::string &name, bool checkPlayerChange) const;
};
