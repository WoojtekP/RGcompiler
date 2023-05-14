#pragma once

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <graph/Action.hpp>
#include <graph/Node.hpp>

class Edge
{
private:
    std::shared_ptr<Node> from_;
    std::shared_ptr<Node> to_;
    std::vector<std::shared_ptr<IAction>> actions_;
    std::vector<std::shared_ptr<Node>> innerNodes_;

    ActionType getActionType() const;
    std::string getActionLeftSide() const;
    std::string getActionRightSide() const;
    bool getActionNegationValue() const;

public:
    Edge(
        const std::shared_ptr<Node> &from,
        const std::shared_ptr<Node> &to,
        const std::vector<std::shared_ptr<IAction>> &actions);
    Edge(
        const std::shared_ptr<Node> &from,
        const std::shared_ptr<Node> &to,
        const std::vector<std::shared_ptr<IAction>> &actions,
        const std::vector<std::shared_ptr<Node>> &innerNodes);
    ~Edge();
    bool operator==(const Edge &edge) const;
    std::string toString() const;
    std::string fromName() const;
    std::string toName() const;
    std::string fullName() const;
    std::string actionToString() const;
    std::shared_ptr<Node> getLeftNode() const;
    std::shared_ptr<Node> getRightNode() const;
    bool isComplementaryTo(const Edge &rhs) const;
    const std::vector<std::shared_ptr<Node>> &getInnerNodes() const;
    const std::vector<std::shared_ptr<IAction>> &getActions() const;
};
