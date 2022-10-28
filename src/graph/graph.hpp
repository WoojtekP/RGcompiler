#include <map>
#include <set>
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

public:
    Node(const nlohmann::json &t);
    std::string toString() const;
    std::string getName() const;
    bool operator==(const Node &rhs) const;
};

class Edge
{
private:
    std::shared_ptr<Node> from_;
    std::shared_ptr<Node> to_;
    std::vector<std::shared_ptr<Action>> actions_;
    std::vector<std::shared_ptr<Node>> innerNodes_;

    ActionType getActionType() const;
    std::string getActionLeftSide() const;
    std::string getActionRightSide() const;
    bool getActionNegationValue() const;

public:
    Edge(
        const std::shared_ptr<Node> &from,
        const std::shared_ptr<Node> &to,
        const std::vector<std::shared_ptr<Action>> &actions);
    Edge(
        const std::shared_ptr<Node> &from,
        const std::shared_ptr<Node> &to,
        const std::vector<std::shared_ptr<Action>> &actions,
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
    const std::vector<std::shared_ptr<Action>> &getActions() const;
};

class Graph
{
    std::vector<int> numberOfIncomingEdges_;
    std::vector<std::string> nodeNames_;
    std::vector<std::vector<std::pair<std::shared_ptr<Edge>, int>>> outgoingEdgesFromNode_;
    std::vector<std::tuple<std::string, std::string, int>> edgeNames_;
    std::map<std::string, int> nodeStringToInt_;
    std::map<std::tuple<std::string, std::string, int>, int> edgeStringToInt_;
    std::map<int, std::shared_ptr<Edge>> edgeIdToEdge_;
    std::map<int, std::shared_ptr<Node>> nodeIdToNode_;
    std::set<std::pair<std::shared_ptr<Edge>, int>> importantEdges_;
    std::set<std::string> importantNodes_;
    std::vector<std::shared_ptr<Edge>> edges_;
    std::vector<std::vector<int>> next_;

    void traverse(int node, std::vector<int> &path, std::vector<std::vector<int>> &paths);
    int getNumberOfIncomingEdges(const std::string &node);
    std::vector<std::string> getTransitions(std::string from);

public:
    ~Graph();
    void initialize();
    void addEdge(std::shared_ptr<Edge> &&edge);
    void addEdge(const std::shared_ptr<Edge> &edge);
    std::vector<std::string> getOutgoingNodesFrom(std::string from);
    std::vector<std::tuple<std::string, std::string, int>> getUnambiguousPathFromNode(
        const std::string &name, bool checkPlayerChange = false);
    const std::vector<std::string> &getNodeNames();
    const std::vector<std::pair<std::shared_ptr<Edge>, int>> &getOutgoingEdgesFrom(std::string from);
    const std::vector<std::tuple<std::string, std::string, int>> &getEdgeNames();
    const std::vector<std::shared_ptr<Action>> &getActions(std::string a, std::string b, int id);
    const std::set<std::pair<std::shared_ptr<Edge>, int>> &getImportantEdges();
    std::string toString();
    std::shared_ptr<Graph> getGraphWithOptimizedPaths();
    int getEdgeId(std::string from, std::string to, int id);
    int getNodeId(std::string name);
    int getNumberOfOutgoingEdges(const std::string &node);
    std::shared_ptr<Edge> getEdge(std::string from, std::string to, int id);
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> generateGraphForPatterns();
    std::shared_ptr<Graph> generateGraphForPattern(std::string from, std::string to);
    bool generatePathFromNodeToNode(
        std::string node,
        std::string finalNode,
        std::vector<std::shared_ptr<Edge>> &edges,
        std::vector<bool> &visited,
        std::vector<bool> &onPathToFinalNode);
};
