#include <string>
#include <vector>


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
    Node *from_     = nullptr;
    Node *to_       = nullptr;
    Action *action_ = nullptr;

public:
    Edge(Node *from, Node *to, Action *action);
    ~Edge();
    std::string toString();
    std::string fromName();
    std::string toName();
    std::string fullName();
    std::string actionToString();
};

class Graph
{
    std::vector<Edge*> edges_;
    std::vector<std::string> getTransitions(std::string from);

public:
    ~Graph();
    void addEdge(Edge *edge);
    std::string toString();
};
