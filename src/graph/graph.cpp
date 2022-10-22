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

std::vector<std::string> Graph::getTransitions(std::string from)
{
    std::vector<std::string> v;

    for (auto &&edge : edges_)
    {
        if (edge->fromName() == from)
        {
            v.push_back(edge->fullName());
        }
    }

    return v;
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

std::string Graph::toString()
{
    std::string graph;

    for (auto &&edge : edges_)
    {
        std::vector<std::string> transitions = getTransitions(edge->toName());

        std::string functionName = edge->fullName();
        std::string functionAction = edge->actionToString();
        std::string functionBody;

        functionBody += "    " + functionAction + "\n";

        for (std::string &name : transitions)
        {
            functionBody += "    " + name + "();\n";
        }

        graph += "void " + functionName + "()\n";
        graph += "{\n";
        graph += functionBody;
        graph += "}\n\n";
    }

    return graph;
}

const std::vector<std::tuple<std::string, std::string, int>> &Graph::getEdgeNames()
{
    return edgeNames_;
}

const std::vector<std::pair<std::shared_ptr<Edge>, int>> &Graph::getOutgoingEdgesFrom(std::string from)
{
    return outgoingEdgesFromNode_[nodeStringToInt_[from]];
}

const std::vector<std::shared_ptr<Action>> &Graph::getActions(std::string a, std::string b, int id)
{
    return edgeIdToEdge_[edgeStringToInt_[std::make_tuple(a, b, id)]]->getActions();
}

const std::vector<std::string> &Graph::getNodeNames()
{
    return nodeNames_;
}

const std::vector<std::shared_ptr<Node>> &Edge::getInnerNodes()
{
    return innerNodes_;
}

std::vector<std::string> Graph::getOutgoingNodesFrom(std::string from)
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

void Graph::traverse(int node, std::vector<int> &path, std::vector<std::vector<int>> &paths)
{
    path.push_back(node);

    if (next_[node].size() != 1 || getNumberOfIncomingEdges(nodeIdToNode_[node]->getName()) != 1)
    {
        return;
    }

    traverse(next_[node].front(), path, paths);
}

std::shared_ptr<Graph> Graph::getGraphWithOptimizedPaths()
{
    std::vector<std::vector<int>> paths;

    for (const auto &name : nodeNames_)
    {
        int u = nodeStringToInt_[name];

        if (getNumberOfIncomingEdges(name) != 1 || next_[u].size() != 1)
        {
            for (int v : next_[u])
            {
                std::vector<int> path {u};

                traverse(v, path, paths);

                paths.push_back(path);
            }
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
            const auto &edge = edgeIdToEdge_[edgeStringToInt_[std::make_tuple(
                nodeIdToNode_[path[i]]->getName(), nodeIdToNode_[path[i + 1]]->getName(), 0)]];

            for (const auto &action : edge->getActions())
            {
                actions.push_back(action);
            }
        }

        std::vector<std::shared_ptr<Node>> innerNodes;

        for (int i = 1; i < path.size() - 1; i++)
        {
            innerNodes.push_back(nodeIdToNode_[path[i]]);

            newGraph->addEdge(std::move(std::make_shared<Edge>(
                nodeIdToNode_[path[i]],
                nodeIdToNode_[path[i + 1]],
                std::vector<std::shared_ptr<Action>> {std::make_shared<Action>()})));
        }

        newGraph->addEdge(
            std::move(std::make_shared<Edge>(nodeIdToNode_[firstNode], nodeIdToNode_[lastNode], actions, innerNodes)));
    }

    newGraph->initialize();

    return newGraph;
}

void Graph::initialize()
{
    std::set<std::string> nodes;

    for (auto &&edge : edges_)
    {
        nodes.insert(edge->fromName());
        nodes.insert(edge->toName());
    }

    int numberOfNodes = nodes.size();

    outgoingEdgesFromNode_.resize(numberOfNodes);

    nodeNames_.insert(nodeNames_.end(), nodes.begin(), nodes.end());

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

        int cnt = countRepetition[std::make_pair(nodeFrom, nodeTo)];
        int val = edgeStringToInt_.size() + shift;

        outgoingEdgesFromNode_[nodeStringToInt_[nodeFrom]].push_back(make_pair(edge, cnt));
        edgeStringToInt_[std::make_tuple(nodeFrom, nodeTo, cnt)] = val;
        edgeIdToEdge_[val] = edge;

        edgeNames_.push_back(std::make_tuple(nodeFrom, nodeTo, cnt));
    }

    next_.resize(numberOfNodes);
    numberOfIncomingEdges_.resize(numberOfNodes, 0);

    for (auto &&edge : edges_)
    {
        next_[nodeStringToInt_[edge->fromName()]].push_back(nodeStringToInt_[edge->toName()]);
        numberOfIncomingEdges_[nodeStringToInt_[edge->toName()]]++;
    }
}

int Graph::getNumberOfOutgoingEdges(const std::string &node)
{
    return next_[nodeStringToInt_[node]].size();
}

int Graph::getNumberOfIncomingEdges(const std::string &node)
{
    return numberOfIncomingEdges_[nodeStringToInt_[node]];
}

int Graph::getNodeId(std::string name)
{
    return nodeStringToInt_[name];
}

int Graph::getEdgeId(std::string from, std::string to, int id)
{
    return edgeStringToInt_[std::tuple(from, to, id)];
}

std::shared_ptr<Edge> Graph::getEdge(std::string from, std::string to, int id)
{
    return edgeIdToEdge_[edgeStringToInt_[std::make_tuple(from, to, id)]];
}
