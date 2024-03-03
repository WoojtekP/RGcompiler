#include "Graph.hpp"

#include <iostream>
#include <queue>
#include <string>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>

Graph::~Graph() {}

void Graph::addEdge(std::shared_ptr<Edge> &&edge)
{
    edges_.emplace_back(std::move(edge));
}

void Graph::addEdge(const std::shared_ptr<Edge> &edge)
{
    edges_.push_back(edge);
}

std::string Graph::toString() const
{
    std::string graph;

    for (auto &&edge : edges_)
    {
        std::cout << edge->fromName() << "--\"";
        for (const auto &action : edge->getActions())
        {
            std::cout << action->toString() << "<br/>";
        }
        std::cout << "\"-->" << edge->toName() << "\n";
    }

    return graph;
}

bool Graph::empty() const
{
    return edges_.empty();
}

const EdgesWithIID &Graph::getAllEdges() const
{
    return edgesWithIID_;
}

const EdgesWithIID &Graph::getOutgoingEdgesFrom(const std::string &from) const
{
    return getOutgoingEdgesFrom(nodeNameToId_.at(from));
}

const EdgesWithIID &Graph::getOutgoingEdgesFrom(int from) const
{
    return outgoingEdgesFromNode_.at(from);
}

const std::vector<std::shared_ptr<Node>> &Graph::getOuterNodes() const
{
    return outerNodes_;
}

const std::vector<std::shared_ptr<Node>> &Graph::getAllNodes() const
{
    return allNodes_;
}

int Graph::getNodeId(const std::shared_ptr<Node> &node)
{
    nodeNameToId_.insert({node->getName(), static_cast<int>(nodeNameToId_.size())});
    return nodeNameToId_[node->getName()];
}

void Graph::insertToNodeIdToNode(const std::shared_ptr<Node> &node)
{
    nodeIdToNode_.insert({getNodeId(node), node});
}

int Graph::getEdgeId(const std::shared_ptr<Edge> &edge, int iid)
{
    size_t shift = nodeNameToId_.size();
    edgeNameToId_.insert({{edge->fromName(), edge->toName(), iid}, static_cast<int>(shift + edgeNameToId_.size())});
    return edgeNameToId_[{edge->fromName(), edge->toName(), iid}];
}

void Graph::insertToEdgeIdToEdge(const std::shared_ptr<Edge> &edge, int iid)
{
    edgeIdToEdge_.insert({getEdgeId(edge, iid), edge});
}

void Graph::initializeNodeIdToNode()
{
    for (auto &&edge : edges_)
    {
        insertToNodeIdToNode(edge->getRightNode());
        insertToNodeIdToNode(edge->getLeftNode());
        for (const auto &innerNode : edge->getInnerNodes())
        {
            insertToNodeIdToNode(innerNode);
        }
    }
}

void Graph::initializeAllNodes()
{
    for (auto &[id, node] : nodeIdToNode_)
    {
        allNodes_.push_back(node);
    }
}

void Graph::initializeEdgeIdToEdgeAndOutgoingEdgesFromNode()
{
    std::map<std::pair<std::string, std::string>, int> countRepetition;

    for (auto &&edge : edges_)
    {
        std::string nodeFrom = edge->fromName();
        std::string nodeTo = edge->toName();

        if (countRepetition.find(std::make_pair(nodeFrom, nodeTo)) == countRepetition.end())
        {
            countRepetition[std::make_pair(nodeFrom, nodeTo)] = 0;
        }
        else
        {
            countRepetition[std::make_pair(nodeFrom, nodeTo)]++;
        }

        int iid = countRepetition[std::make_pair(nodeFrom, nodeTo)];
        insertToEdgeIdToEdge(edge, iid);
        outgoingEdgesFromNode_[nodeNameToId_.at(nodeFrom)].push_back({edge, iid});
        edgesWithIID_.push_back({edge, iid});
    }

    // Insert empty list of outgoing edges for nodes without outgoing edges
    for (const auto &[nodeName, nodeId] : nodeNameToId_)
    {
        outgoingEdgesFromNode_.insert({nodeId, {}});
    }
}

void Graph::initialize()
{
    std::set<std::shared_ptr<Node>, ByNameComparator<std::shared_ptr<Node>>> outerNodes;

    for (auto &&edge : edges_)
    {
        outerNodes.insert(edge->getLeftNode());
        outerNodes.insert(edge->getRightNode());
    }
    outerNodes_.insert(outerNodes_.end(), outerNodes.begin(), outerNodes.end());

    initializeNodeIdToNode();
    initializeAllNodes();
    initializeEdgeIdToEdgeAndOutgoingEdgesFromNode();
}

int Graph::getNodeId(const std::string &name) const
{
    return nodeNameToId_.at(name);
}

int Graph::getEdgeId(const std::string &from, const std::string &to, int iid) const
{
    return edgeNameToId_.at(std::tuple(from, to, iid));
}

std::shared_ptr<Edge> Graph::getEdge(const std::string &from, const std::string &to, int iid) const
{
    return edgeIdToEdge_.at(edgeNameToId_.at(std::make_tuple(from, to, iid)));
}

std::shared_ptr<Node> Graph::getNode(int nodeid) const
{
    return nodeIdToNode_.at(nodeid);
}

std::optional<int> Graph::getNodeIdOptional(const std::string &name) const
{
    return getOptional(nodeNameToId_, name);
}

std::optional<std::shared_ptr<Node>> Graph::getNodeOptional(int nodeid) const
{
    return getOptional(nodeIdToNode_, nodeid);
}

std::optional<int> Graph::getEdgeIdOptional(const std::string &from, const std::string &to, int iid) const
{
    return getOptional(edgeNameToId_, std::tuple(from, to, iid));
}

std::optional<std::shared_ptr<Edge>> Graph::getEdgeOptional(
    const std::string &from, const std::string &to, int iid) const
{
    return getOptional(edgeIdToEdge_, edgeNameToId_.at(std::make_tuple(from, to, iid)));
}

int Graph::getMaximalNodeId() const
{
    return nodeNameToId_.size();
}

int Graph::getNumberOfNodes() const
{
    return nodeNameToId_.size();
}