#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <graph/Edge.hpp>
#include <graph/Node.hpp>

using EdgesWithIID = std::vector<std::pair<std::shared_ptr<Edge>, int>>;

template<typename T>
struct ByNameComparator
{
    inline bool operator()(const T &lhs, const T &rhs) const { return lhs->toString() < rhs->toString(); };
};

class Graph
{
    std::vector<std::shared_ptr<Edge>> edges_;
    // Pair of edge and its internal id
    EdgesWithIID edgesWithIID_;
    std::map<int, std::shared_ptr<Node>> nodeIdToNode_;
    std::map<std::string, int> nodeNameToId_;
    // Visible nodes
    std::vector<std::shared_ptr<Node>> outerNodes_;
    // Visible nodes and nodes hidden in edges
    std::vector<std::shared_ptr<Node>> allNodes_;
    // NodeId to vector of outgoing edges {edge , internal id}
    std::map<int, EdgesWithIID> outgoingEdgesFromNode_;
    // {NodeFrom name, NodeTo name, internal id} to edge id
    std::map<std::tuple<std::string, std::string, int>, int> edgeNameToId_;
    std::map<int, std::shared_ptr<Edge>> edgeIdToEdge_;

    void insertToEdgeIdToEdge(const std::shared_ptr<Edge> &edge, int iid);
    void insertToNodeIdToNode(const std::shared_ptr<Node> &node);
    void initializeEdgeIdToEdgeAndOutgoingEdgesFromNode();
    void initializeNodeIdToNode();
    void initializeAllNodes();
    int getNodeId(const std::shared_ptr<Node> &node);
    int getEdgeId(const std::shared_ptr<Edge> &edge, int iid);
    template<typename Map, typename Key>
    std::optional<typename Map::mapped_type> getOptional(const Map &container, const Key &key) const
    {
        auto iter = container.find(key);
        if (iter == container.end())
        {
            return std::nullopt;
        }
        return iter->second;
    }

public:
    ~Graph();
    void initialize();
    void addEdge(std::shared_ptr<Edge> &&edge);
    void addEdge(const std::shared_ptr<Edge> &edge);
    bool empty() const;
    std::string toString() const;
    const std::vector<std::shared_ptr<Node>> &getOuterNodes() const;
    const std::vector<std::shared_ptr<Node>> &getAllNodes() const;
    const EdgesWithIID &getOutgoingEdgesFrom(const std::string &from) const;
    const EdgesWithIID &getOutgoingEdgesFrom(int from) const;
    const EdgesWithIID &getAllEdges() const;
    int getEdgeId(const std::string &from, const std::string &to, int iid) const;
    int getNodeId(const std::string &name) const;
    std::shared_ptr<Edge> getEdge(const std::string &from, const std::string &to, int iid) const;
    std::shared_ptr<Node> getNode(int nodeid) const;
    std::optional<int> getEdgeIdOptional(const std::string &from, const std::string &to, int iid) const;
    std::optional<int> getNodeIdOptional(const std::string &name) const;
    std::optional<std::shared_ptr<Edge>> getEdgeOptional(const std::string &from, const std::string &to, int iid) const;
    std::optional<std::shared_ptr<Node>> getNodeOptional(int nodeid) const;
    int getNumberOfNodes() const;
    int getMaximalNodeId() const;
};
