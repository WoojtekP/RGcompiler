#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include <graph/action.hpp>
#include <graph/graph.hpp>
#include <parser/parser.hpp>

Binding::Binding(std::string variableName, std::string iteratedType)
: variableName_(variableName), iteratedType_(iteratedType)
{}

std::string Binding::toString() const
{
    return "(" + iteratedType_ + ":" + variableName_ + ")";
}

Node::Node(const nlohmann::json &t)
{
    name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

    if (const auto &binding = Parser::getPartFromParts(t, "Binding"))
    {
        throw std::logic_error(
            "Binds are not implemented. Use --expandGeneratorNodes to remove them when generating AST");
    }
}

std::string Node::getName() const
{
    return name_;
}

std::string Node::toString() const
{
    return name_;
}

bool Node::operator==(const Node &rhs) const
{
    return name_ == rhs.name_;
}

Edge::Edge(
    const std::shared_ptr<Node> &from,
    const std::shared_ptr<Node> &to,
    const std::vector<std::shared_ptr<Action>> &actions)
: Edge(from, to, actions, {})
{}

Edge::Edge(
    const std::shared_ptr<Node> &from,
    const std::shared_ptr<Node> &to,
    const std::vector<std::shared_ptr<Action>> &actions,
    const std::vector<std::shared_ptr<Node>> &innerNodes)
: from_(from), to_(to), actions_(actions.begin(), actions.end()), innerNodes_(innerNodes.begin(), innerNodes.end()) {};

Edge::~Edge() {}

bool Edge::operator==(const Edge &edge) const
{
    const auto &innerNodes = edge.getInnerNodes();

    if (fromName() != edge.fromName() || toName() != edge.toName() || innerNodes_.size() != innerNodes.size())
    {
        return false;
    }

    for (size_t i = 0; i < innerNodes_.size(); i++)
    {
        if (innerNodes_[i] != innerNodes[i])
        {
            return false;
        }
    }
    return true;
}

std::string Edge::toString() const
{
    std::string actions;

    for (const auto &action : actions_)
    {
        actions += ", " + action->toString();
    }

    return "<" + from_->toString() + ", " + to_->toString() + actions + ">";
}

std::string Edge::fromName() const
{
    if (from_)
    {
        return from_->toString();
    }

    return "";
}

std::string Edge::toName() const
{
    if (to_)
    {
        return to_->toString();
    }

    return "";
}

Graph::~Graph() {}

void Graph::addEdge(std::shared_ptr<Edge> &&edge)
{
    edges_.emplace_back(std::move(edge));
}

void Graph::addEdge(const std::shared_ptr<Edge> &edge)
{
    edges_.push_back(edge);
}

std::string Edge::fullName() const
{
    if (from_ && to_)
    {
        return "edge_" + fromName() + "_" + toName();
    }

    return "";
}

std::string Edge::actionToString() const
{
    std::string actions;

    for (const auto &action : actions_)
    {
        actions = action->toString() + ";\n";
    }

    return actions;
}

ActionType Edge::getActionType() const
{
    return actions_.front()->getType();
}

std::string Edge::getActionLeftSide() const
{
    return actions_.front()->getLeftSide();
}

std::string Edge::getActionRightSide() const
{
    return actions_.front()->getRightSide();
}

bool Edge::getActionNegationValue() const
{
    return actions_.front()->getNegated();
}

bool Edge::isComplementaryTo(const Edge &rhs) const
{
    return *from_ == *rhs.from_ && getActionType() == rhs.getActionType() &&
           getActionLeftSide() == rhs.getActionLeftSide() && getActionRightSide() == rhs.getActionRightSide() &&
           getActionNegationValue() != rhs.getActionNegationValue();
}

const std::vector<std::shared_ptr<Action>> &Edge::getActions() const
{
    return actions_;
}

std::shared_ptr<Node> Edge::getLeftNode() const
{
    return from_;
}

std::shared_ptr<Node> Edge::getRightNode() const
{
    return to_;
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

const std::vector<std::tuple<std::string, std::string, int>> &Graph::getEdgeNames() const
{
    return edgeNames_;
}

std::set<std::pair<std::shared_ptr<Edge>, int>> Graph::getEdgeWithActionChangePlayer()
{
    std::set<std::string> nodes;
    std::set<std::pair<std::shared_ptr<Edge>, int>> edges;

    for (auto &state : getOuterNodeNames())
    {
        for (const auto &[edge, iid] : getOutgoingEdgesFrom(state))
        {
            for (const auto &action : edge->getActions())
            {
                if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
                {
                    if (nodes.find(edge->toName()) == nodes.end())
                    {
                        edges.insert(std::make_pair(edge, iid));
                        nodes.insert(edge->toName());
                    }
                }
            }
        }
    }

    return edges;
}

const std::vector<std::pair<std::shared_ptr<Edge>, int>> &Graph::getOutgoingEdgesFrom(std::string from) const
{
    return outgoingEdgesFromNode_[nodeStringToInt_.at(from)];
}

const std::vector<std::shared_ptr<Action>> &Graph::getActions(std::string fromName, std::string toName, int iid) const
{
    return edgeIdToEdge_.at(edgeStringToInt_.at(std::make_tuple(fromName, toName, iid)))->getActions();
}

const std::vector<std::string> &Graph::getOuterAndInnerNodeNames() const
{
    return outerAndInnerNodeNames_;
}

const std::vector<std::string> &Graph::getOuterNodeNames() const
{
    return outerNodeNames_;
}

const std::vector<std::shared_ptr<Node>> &Edge::getInnerNodes() const
{
    return innerNodes_;
}

std::vector<std::string> Graph::getOutgoingNodesFrom(std::string from) const
{
    std::vector<std::string> outgingNodes;

    for (auto &&edge : edges_)
    {
        if (edge->fromName() == from)
        {
            outgingNodes.push_back(edge->toName());
        }
    }

    return outgingNodes;
}

std::vector<std::tuple<std::string, std::string, int>> Graph::getUnambiguousPathFromNode(
    const std::string &name, bool checkPlayerChange) const
{
    std::string node = name;
    std::vector<std::tuple<std::string, std::string, int>> path;

    while (getNumberOfOutgoingEdges(node) == 1)
    {
        const auto &[edge, iid] = getOutgoingEdgesFrom(node).back();

        path.push_back(std::make_tuple(edge->fromName(), edge->toName(), iid));
        if (checkPlayerChange)
        {
            for (const auto &action : edge->getActions())
            {
                if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
                {
                    return path;
                }
            }
        }

        if (node == edge->toName())
        {
            break;
        }

        node = edge->toName();
    }

    return path;
}

void Graph::traverse(
    int node, std::vector<int> &path, std::vector<std::vector<int>> &paths, std::vector<bool> &visited) const
{
    const auto &action =
        getEdge(nodeIdToNode_.at(path.back())->getName(), nodeIdToNode_.at(node)->getName(), 0)->getActions().back();
    path.push_back(node);

    if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
    {
        return;
    }

    if (nodesFromNode_[node].size() != 1 || getNumberOfIncomingEdges(nodeIdToNode_.at(node)->getName()) != 1)
    {
        return;
    }

    visited[node] = true;
    traverse(nodesFromNode_[node].front(), path, paths, visited);
}

void Graph::traverseCycle(int node, std::vector<int> &path, std::vector<bool> &visited) const
{
    path.push_back(node);

    if (visited[node])
    {
        return;
    }

    visited[node] = true;

    traverseCycle(nodesFromNode_[node].back(), path, visited);
}

std::shared_ptr<Graph> Graph::getGraphWithOptimizedPaths() const
{
    std::vector<std::vector<int>> paths;
    std::vector<bool> visited(nodeIdToNode_.size(), false);

    for (const auto &name : getOuterAndInnerNodeNames())
    {
        int u = nodeStringToInt_.at(name);

        if (getNumberOfIncomingEdges(name) != 1 || nodesFromNode_[u].size() != 1)
        {
            visited[u] = true;
            for (int v : nodesFromNode_[u])
            {
                std::vector<int> path {u};

                traverse(v, path, paths, visited);

                paths.push_back(path);

                while (getNumberOfIncomingEdges(nodeIdToNode_.at(path.back())->getName()) == 1 &&
                       nodesFromNode_[path.back()].size() == 1)
                {
                    visited[path.back()] = true;
                    int node = path.back();
                    path.clear();
                    path.push_back(node);
                    traverse(nodesFromNode_[node].back(), path, paths, visited);
                    paths.push_back(path);
                }
            }
        }
    }

    for (const auto &name : getOuterAndInnerNodeNames())
    {
        int u = nodeStringToInt_.at(name);

        if (!visited[u])
        {
            std::vector<int> path;

            traverseCycle(u, path, visited);

            paths.push_back(path);
        }
    }

    std::shared_ptr<Graph> newGraph = std::make_shared<Graph>();

    for (const auto &path : paths)
    {
        int firstNode = path.front();
        int lastNode = path.back();

        std::vector<std::shared_ptr<Action>> actions;

        for (int i = 0; i < path.size() - 1; i++)
        {
            const auto &edge = edgeIdToEdge_.at(edgeStringToInt_.at(
                std::make_tuple(nodeIdToNode_.at(path[i])->getName(), nodeIdToNode_.at(path[i + 1])->getName(), 0)));

            for (const auto &action : edge->getActions())
            {
                actions.push_back(action);
            }
        }

        std::vector<std::shared_ptr<Node>> innerNodes;

        for (int i = 1; i < path.size() - 1; i++)
        {
            innerNodes.push_back(nodeIdToNode_.at(path[i]));
        }

        newGraph->addEdge(std::move(
            std::make_shared<Edge>(nodeIdToNode_.at(firstNode), nodeIdToNode_.at(lastNode), actions, innerNodes)));
    }

    newGraph->initialize();

    return newGraph;
}

void Graph::initialize()
{
    std::set<std::string> nodes;
    std::set<std::string> outerNodes;

    for (auto &&edge : edges_)
    {
        nodes.insert(edge->fromName());
        nodes.insert(edge->toName());
        outerNodes.insert(edge->fromName());
        outerNodes.insert(edge->toName());

        for (const auto &innerNode : edge->getInnerNodes())
        {
            nodes.insert(innerNode->getName());
        }
    }

    int numberOfNodes = nodes.size();

    outgoingEdgesFromNode_.resize(numberOfNodes);

    outerAndInnerNodeNames_.insert(outerAndInnerNodeNames_.end(), nodes.begin(), nodes.end());
    outerNodeNames_.insert(outerNodeNames_.end(), outerNodes.begin(), outerNodes.end());

    for (auto &node : nodes)
    {
        nodeStringToInt_[node] = nodeStringToInt_.size();
    }

    for (auto &&edge : edges_)
    {
        if (nodes.find(edge->fromName()) != nodes.end())
        {
            nodes.erase(edge->fromName());
            nodeIdToNode_[nodeStringToInt_[edge->fromName()]] = edge->getLeftNode();
        }

        if (nodes.find(edge->toName()) != nodes.end())
        {
            nodes.erase(edge->toName());
            nodeIdToNode_[nodeStringToInt_[edge->toName()]] = edge->getRightNode();
        }
    }

    int shift = nodeStringToInt_.size();

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
        int val = edgeStringToInt_.size() + shift;

        outgoingEdgesFromNode_[nodeStringToInt_[nodeFrom]].push_back(make_pair(edge, iid));
        edgeStringToInt_[std::make_tuple(nodeFrom, nodeTo, iid)] = val;
        edgeIdToEdge_[val] = edge;

        edgeNames_.push_back(std::make_tuple(nodeFrom, nodeTo, iid));
    }

    nodesFromNode_.resize(numberOfNodes);
    numberOfIncomingEdges_.resize(numberOfNodes, 0);

    for (auto &&edge : edges_)
    {
        nodesFromNode_[nodeStringToInt_[edge->fromName()]].push_back(nodeStringToInt_[edge->toName()]);
        numberOfIncomingEdges_[nodeStringToInt_[edge->toName()]]++;
    }
}

int Graph::getNumberOfOutgoingEdges(const std::string &node) const
{
    return nodesFromNode_[nodeStringToInt_.at(node)].size();
}

int Graph::getNumberOfIncomingEdges(const std::string &node) const
{
    return numberOfIncomingEdges_[nodeStringToInt_.at(node)];
}

int Graph::getNodeId(std::string name) const
{
    return nodeStringToInt_.at(name);
}

int Graph::getEdgeId(std::string from, std::string to, int iid) const
{
    return edgeStringToInt_.at(std::tuple(from, to, iid));
}

std::shared_ptr<Edge> Graph::getEdge(std::string from, std::string to, int iid) const
{
    return edgeIdToEdge_.at(edgeStringToInt_.at(std::make_tuple(from, to, iid)));
}

bool Graph::generatePathFromNodeToNode(
    std::string node,
    std::string finalNode,
    std::vector<std::shared_ptr<Edge>> &edges,
    std::vector<bool> &visited,
    std::vector<bool> &onPathToFinalNode,
    const std::set<int> &bannedEdges) const
{
    if (node == finalNode)
    {
        return true;
    }

    bool havePathToFinalNode = false;

    for (const auto &[edge, iid] : getOutgoingEdgesFrom(node))
    {
        int edgeId = edgeStringToInt_.at(std::make_tuple(edge->fromName(), edge->toName(), iid));
        int edgeShiftedId = edgeId - nodeStringToInt_.size();

        if (bannedEdges.find(edgeId) != bannedEdges.end())
        {
            continue;
        }

        if (!visited[edgeShiftedId])
        {
            visited[edgeShiftedId] = true;
            if (generatePathFromNodeToNode(edge->toName(), finalNode, edges, visited, onPathToFinalNode, bannedEdges))
            {
                havePathToFinalNode = true;
                edges.push_back(edge);
            }
        }
        else if (onPathToFinalNode[nodeStringToInt_.at(edge->toName())])
        {
            havePathToFinalNode = true;
        }
    }

    if (havePathToFinalNode)
    {
        onPathToFinalNode[nodeStringToInt_.at(node)] = true;
    }

    return havePathToFinalNode;
}

std::shared_ptr<Graph> Graph::generateGraphForPattern(
    std::string from, std::string to, const std::set<int> &bannedEdges) const
{
    std::shared_ptr<Graph> graph = std::make_shared<Graph>();

    std::vector<std::shared_ptr<Edge>> edges;

    std::vector<bool> visited(edges_.size(), false);
    std::vector<bool> nodesOnPathToFinalNode(nodeStringToInt_.size(), false);
    nodesOnPathToFinalNode[nodeStringToInt_.at(to)] = true;

    generatePathFromNodeToNode(from, to, edges, visited, nodesOnPathToFinalNode, bannedEdges);

    for (const auto &edge : edges)
    {
        graph->addEdge(edge);
    }

    return graph;
}

std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> Graph::generateGraphForPatterns(
    ActionType actionType) const
{
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternGraphs;

    std::set<std::pair<std::string, std::string>> patterns;

    for (const auto &edge : edges_)
    {
        const auto &action = edge->getActions().front();

        if (action->getType() == actionType &&
            patterns.find(std::make_pair(action->getLeftSide(), action->getRightSide())) == patterns.end())
        {
            patterns.insert(std::make_pair(action->getLeftSide(), action->getRightSide()));
        }
    }

    for (const auto &[from, to] : patterns)
    {
        patternGraphs.push_back(std::make_tuple(from, to, generateGraphForPattern(from, to)));
    }

    return patternGraphs;
}

std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> Graph::generateGraphsForApplyAnyMove() const
{
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> graphs;
    std::map<std::string, std::set<int>> edgesWithActionChangePlayerForToNode;
    std::set<int> edgesWithActionChangePlayer;

    for (const auto &v : edgeStringToInt_)
    {
        const auto &[fromName, toName, iid] = v.first;

        auto edge = getEdge(fromName, toName, iid);
        const auto &action = edge->getActions().back();

        if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player")
        {
            int edgeId = v.second;
            edgesWithActionChangePlayer.insert(edgeId);
            auto &p = edgesWithActionChangePlayerForToNode[toName];
            p.insert(edgeId);
        }
    }

    auto getBannedEdges = [&edgesWithActionChangePlayer](const std::set<int> &goodEdges) {
        std::set<int> res;

        for (int edge : edgesWithActionChangePlayer)
        {
            if (goodEdges.find(edge) == goodEdges.end())
            {
                res.insert(edge);
            }
        }

        return res;
    };

    for (const auto &fromName : getNodesBeforeWhichPlayerChangeToKeeper())
    {
        for (const auto &toName : nodesToPlayerChangeOrEnd(fromName))
        {
            std::set<int> bannedEdges = getBannedEdges(edgesWithActionChangePlayerForToNode[toName]);
            std::shared_ptr<Graph> graph = generateGraphForPattern(fromName, toName, bannedEdges);

            if (!graph->empty())
            {
                graphs.push_back({fromName, toName, graph});
            }
        }
    }

    return graphs;
}

std::pair<std::shared_ptr<Edge>, int> Graph::getUnambiguousNotEmptyEdge(const std::string &name) const
{
    std::string stateFrom = name;
    std::string stateTo;

    while (nodesFromNode_[nodeStringToInt_.at(stateFrom)].size() == 1)
    {
        stateTo = nodeIdToNode_.at(nodesFromNode_[nodeStringToInt_.at(stateFrom)].back())->getName();
        for (const auto &action : getActions(stateFrom, stateTo, 0))
        {
            if (action->getType() == ActionType::Assignment)
            {
                return std::make_pair(edgeIdToEdge_.at(edgeStringToInt_.at(std::make_tuple(stateFrom, stateTo, 0))), 0);
            }
        }
        stateFrom = stateTo;
    }

    return std::make_pair(nullptr, -1);
}

std::vector<std::string> Graph::getNodesBeforeWhichPlayerChangeToKeeper() const
{
    std::set<std::string> nodes({"begin"});
    for (const auto &edge : edges_)
    {
        const auto &action = edge->getActions().back();
        // TODO: We need better way to check if keeper changed
        if (action->getType() == ActionType::Assignment && action->getLeftSide() == "player" &&
            (action->getRightSide() == "static_cast<PlayerOrKeeper>(keeper)" || action->getRightSide() == "keeper"))
        {
            nodes.insert(edge->toName());
        }
    }

    return std::vector<std::string>(nodes.begin(), nodes.end());
}

std::vector<std::string> Graph::nodesToPlayerChangeOrEnd(const std::string &nodeName) const
{
    std::set<std::string> visited;
    std::string node = nodeName;
    std::vector<std::string> nodes;
    std::queue<std::string> nodesToVisit({node});
    while (!nodesToVisit.empty())
    {
        std::string node = nodesToVisit.front();
        nodesToVisit.pop();
        for (const auto &[edge, iid] : getOutgoingEdgesFrom(node))
        {
            if (visited.find(edge->toName()) == visited.end())
            {
                visited.insert(edge->toName());
                const auto &action = edge->getActions().back();
                if ((action->getType() == ActionType::Assignment && action->getLeftSide() == "player") ||
                    edge->toName() == "end")
                {
                    nodes.push_back(edge->toName());
                    continue;
                }
                nodesToVisit.push(edge->toName());
            }
        }
    }

    return nodes;
}

void Graph::getVariablesInPatternGraphs(std::map<std::string, int> &result) const
{
    for (const auto &edge : edges_)
    {
        for (const auto &action : edge->getActions())
        {
            const auto &variable = action->getLeftSide();
            if (action->getType() == ActionType::Assignment && (result.find(variable) == result.end()))
            {
                result.insert({variable, result.size()});
            }
        }
    }
}