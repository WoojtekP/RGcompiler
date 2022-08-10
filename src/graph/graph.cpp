#include <string>

#include <nlohmann/json.hpp>

#include <graph/action.hpp>
#include <graph/graph.hpp>
#include <parser/parser.hpp>

Binding::Binding(std::string variableName, std::string iteratedType)
: variableName_(variableName), iteratedType_(iteratedType)
{}

std::string Binding::toString()
{
    return "(" + iteratedType_ + ":" + variableName_ + ")";
}

Node::Node(const nlohmann::json &t)
{
    name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

    // TODO: we should parse more than one binding
    const auto &binding = Parser::getPartFromParts(t, "Binding");

    if (binding)
    {
        bindings_.emplace_back((*binding).get()["identifier"], (*binding).get()["type"]["identifier"]);
    }
}

std::string Node::toString()
{
    std::string bindings;

    for (Binding &binding : bindings_)
    {
        bindings += binding.toString();
    }

    return name_ + bindings;
}

Edge::Edge(std::unique_ptr<Node> &&from, std::unique_ptr<Node> &&to, std::unique_ptr<Action> &&action)
: from_(std::move(from)), to_(std::move(to)), action_(std::move(action)) {};

Edge::~Edge() {}

std::string Edge::toString()
{
    return "<" + from_->toString() + ", " + to_->toString() + ", " + action_->toString() + ">";
}

std::string Edge::fromName()
{
    if (from_)
    {
        return from_->toString();
    }

    return "";
}

std::string Edge::toName()
{
    if (to_)
    {
        return to_->toString();
    }

    return "";
}

Graph::~Graph() {}

void Graph::addEdge(std::unique_ptr<Edge> &&edge)
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

std::string Edge::fullName()
{
    if (from_ && to_)
    {
        return "edge_" + fromName() + "_" + toName();
    }

    return "";
}

std::string Edge::actionToString()
{
    if (action_)
    {
        return action_->toString();
    }

    return "";
}

ActionType Edge::getActionType()
{
    return action_->getType();
}

std::string Edge::getActionLeftSide()
{
    return action_->getLeftSide();
}

std::string Edge::getActionRightSide()
{
    return action_->getRightSide();
}

bool Edge::getActionNegationValue()
{
    return action_->getNegated();
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

// TODO Belowed functions works in O(n) time, they should be changed to constant time
// after mapping node names from string to int is done

ActionType Graph::getActionType(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->getActionType();
        }
    }

    return ActionType::Skip;
}

bool Graph::getActionNegationValue(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->getActionNegationValue();
        }
    }

    return "";
}

std::string Graph::getActionLeftSide(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->getActionLeftSide();
        }
    }

    return "";
}

std::string Graph::getActionRightSide(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->getActionRightSide();
        }
    }

    return "";
}

std::string Graph::getAction(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->actionToString();
        }
    }

    return "";
}

std::string Graph::getToName(std::string edgeName)
{
    for (auto &&edge : edges_)
    {
        if (edge->fullName() == edgeName)
        {
            return edge->toName();
        }
    }

    return "";
}

std::vector<std::string> Graph::getEdgeNames()
{
    std::vector<std::string> v;

    for (auto &&edge : edges_)
    {
        v.push_back(edge->fullName());
    }

    return v;
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

std::vector<std::string> Graph::getNodeNames()
{
    std::set<std::string> nodes;

    for (auto &&edge : edges_)
    {
        nodes.insert(edge->fromName());
        nodes.insert(edge->toName());
    }

    return {nodes.begin(), nodes.end()};
}
