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
    std::string toString() const;
};

class Node
{
    std::string name_;
    std::vector<Binding> bindings_;

public:
    Node(const nlohmann::json &t);
    std::string toString() const;
    bool operator==(const Node& rhs) const;
};

class Edge
{
private:
    std::unique_ptr<Node> from_;
    std::unique_ptr<Node> to_;
    std::unique_ptr<Action> action_;

public:
    Edge(std::unique_ptr<Node> &&from, std::unique_ptr<Node> &&to, std::unique_ptr<Action> &&action);
    ~Edge();
    std::string toString() const;
    std::string fromName() const;
    std::string toName() const;
    std::string fullName() const;
    std::string actionToString() const;
    ActionType getActionType() const;
    std::string getActionLeftSide() const;
    std::string getActionRightSide() const;
    bool getActionNegationValue() const;
    bool isComplementaryTo(const Edge& rhs) const;
};

class Graph
{
    std::vector<std::shared_ptr<Edge>> edges_;
    std::vector<std::string> getTransitions(std::string from);

public:
    ~Graph();
    void addEdge(std::shared_ptr<Edge> &&edge);
    std::string toString();
    std::vector<std::string> getNodeNames();
    std::vector<std::string> getOutgoingNodesFrom(std::string from);
    std::vector<std::shared_ptr<Edge>> getOutgoingEdgesFrom(std::string from);
    std::vector<std::pair<std::string, std::string>> getEdgeNames();
    ActionType getActionType(std::string stateFrom, std::string stateTo);
    std::string getActionLeftSide(std::string stateFrom, std::string stateTo);
    std::string getActionRightSide(std::string stateFrom, std::string stateTo);
    std::string getAction(std::string stateFrom, std::string stateTo);
    std::string getToName(std::string stateFrom, std::string stateTo);
    bool getActionNegationValue(std::string stateFrom, std::string stateTo);
};
