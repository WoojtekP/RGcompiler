#pragma once
#include <compiler/graphOperations/BaseOperator.hpp>

class PragmaDisjointOperator : public BaseOperator
{
    std::map<std::string, std::vector<std::string>> nodeNameToOrderOfDisjointNodes_;
    std::set<std::string> exhaustive_;

public:
    PragmaDisjointOperator(const std::shared_ptr<Graph> &graph);
    std::set<std::string> getNodes() const;
    void init(const Parser &parser);
    bool isDisjoint(const std::string &node) const;
    bool isExhaustive(const std::string &node) const;
    const std::vector<std::string> &getNodeNames(const std::string &nodeName) const;
};
