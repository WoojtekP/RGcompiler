#include <map>
#include <queue>
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

class Graph
{
    std::vector<int> numberOfIncomingEdges_;
    std::vector<std::string> outerAndInnerNodeNames_;
    std::vector<std::string> outerNodeNames_;
    std::vector<std::vector<std::pair<std::shared_ptr<Edge>, int>>> outgoingEdgesFromNode_;
    std::vector<std::tuple<std::string, std::string, int>> edgeNames_;
    std::map<std::string, int> nodeStringToInt_;
    std::map<std::tuple<std::string, std::string, int>, int> edgeStringToInt_;
    std::map<int, std::shared_ptr<Edge>> edgeIdToEdge_;
    std::map<int, std::shared_ptr<Node>> nodeIdToNode_;
    std::vector<std::shared_ptr<Edge>> edges_;
    std::vector<std::vector<int>> nodesFromNode_;

    std::vector<std::string> getNodesBeforeWhichPlayerChangeToKeeper() const;
    std::vector<std::string> nodesToPlayerChangeOrEnd(const std::string &nodeName) const;
    std::shared_ptr<Graph> generateGraphForPattern(
        std::string from, std::string to, const std::set<int> &bannedEdges = std::set<int>()) const;
    void traverseCycle(int node, std::vector<int> &path, std::vector<bool> &visited) const;
    void traverse(
        int node, std::vector<int> &path, std::vector<std::vector<int>> &paths, std::vector<bool> &visited) const;
    bool generatePathFromNodeToNode(
        std::string node,
        std::string finalNode,
        std::vector<std::shared_ptr<Edge>> &edges,
        std::vector<bool> &visited,
        std::vector<bool> &onPathToFinalNode,
        const std::set<int> &bannedEdges) const;

public:
    ~Graph();
    void initialize();
    void addEdge(std::shared_ptr<Edge> &&edge);
    void addEdge(const std::shared_ptr<Edge> &edge);
    bool empty() const;
    std::vector<std::string> getOutgoingNodesFrom(std::string from) const;
    std::vector<std::tuple<std::string, std::string, int>> getUnambiguousPathFromNode(
        const std::string &name, bool checkPlayerChange = false) const;
    const std::vector<std::string> &getOuterAndInnerNodeNames() const;
    const std::vector<std::string> &getOuterNodeNames() const;
    const std::vector<std::pair<std::shared_ptr<Edge>, int>> &getOutgoingEdgesFrom(std::string from) const;
    const std::vector<std::tuple<std::string, std::string, int>> &getEdgeNames() const;
    const std::vector<std::shared_ptr<IAction>> &getActions(std::string fromName, std::string toName, int iid) const;
    std::set<std::pair<std::shared_ptr<Edge>, int>> getEdgeWithActionChangePlayer();
    std::string toString() const;
    std::shared_ptr<Graph> getGraphWithOptimizedPaths() const;
    int getEdgeId(std::string from, std::string to, int iid) const;
    int getNodeId(std::string name) const;
    int getNumberOfOutgoingEdges(const std::string &node) const;
    int getNumberOfIncomingEdges(const std::string &node) const;
    std::shared_ptr<Edge> getEdge(std::string from, std::string to, int iid) const;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> generateGraphForPatterns(
        ActionType actionType) const;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> generateGraphsForApplyAnyMove() const;
    void getVariablesInPatternGraphs(std::set<std::string> &result) const;
    std::pair<std::shared_ptr<Edge>, int> getUnambiguousNotEmptyEdge(const std::string &name) const;
    int getMaximalNodeId() const;
};
