#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <graph/action.hpp>


class Binding
{
    std::string variableName_;
    std::string iteratedType_;

public:
    Binding(std::string variableName, std::string iteratedType);
    std::string toString();
};

class Node
{
    std::string name_;
    std::vector<Binding> bindings_;

public:
    Node(const nlohmann::json& t);
    std::string toString();
};

class Edge
{
private:
    std::unique_ptr<Node> from_;
    std::unique_ptr<Node> to_;
    std::unique_ptr<Action> action_;

public:
    Edge(std::unique_ptr<Node> &&from, std::unique_ptr<Node> &&to,
        std::unique_ptr<Action> &&action);
    ~Edge();
    std::string toString();
    std::string fromName();
    std::string toName();
    std::string fullName();
    std::string actionToString();
    ActionType getActionType();
    std::string getActionLeftSide();
    std::string getActionRightSide();
    bool getActionNegationValue();
};

class Graph
{
    std::vector<std::unique_ptr<Edge>> edges_;
    std::vector<std::string> getTransitions(std::string from);

public:
    ~Graph();
    void addEdge(std::unique_ptr<Edge> &&edge);
    std::string toString();
    std::vector<std::string> getNodeNames();
    std::vector<std::string> getOutgoingNodesFrom(std::string from);
    std::vector<std::string> getEdgeNames();
    ActionType getActionType(std::string edgeName);
    std::string getActionLeftSide(std::string edgeName);
    std::string getActionRightSide(std::string edgeName);
    std::string getAction(std::string edgeName);
    std::string getToName(std::string edgeName);
    bool getActionNegationValue(std::string edgeName);
};
