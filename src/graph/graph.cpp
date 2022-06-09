#include <string>
#include <iostream>

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

Node::Node(const nlohmann::json& t)
{
    name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

    // TODO: we should parse more than one binding
    const auto& binding = Parser::getPartFromParts(t, "Binding");

    if (binding)
    {
        bindings_.emplace_back((*binding).get()["identifier"], (*binding).get()["type"]["identifier"]);
    }
}

std::string Node::toString()
{
    std::string bindings;

    for (Binding& binding : bindings_)
    {
        bindings += binding.toString();
    }

    return name_ + bindings;
}

Edge::Edge(Node *from, Node *to, Action *action) :
    from_(from), to_(to), action_(action)
{
};

Edge::~Edge()
{
    delete from_;
    delete to_;
    delete action_;
}

std::string Edge::toString()
{
    return "<" + from_ -> toString() + ", " + to_ -> toString() + ", " + action_ -> toString() + ">";
}

std::string Edge::fromName()
{
    if (from_)
    {
        return from_ -> toString();
    }

    return "";
}

std::string Edge::toName()
{
    if (to_)
    {
        return to_ -> toString();
    }

    return "";
}

Graph::~Graph()
{
    for (Edge *edge : edges_)
    {
        delete edge;
    }
}

void Graph::addEdge(Edge *edge)
{
    edges_.emplace_back(edge);
}

std::vector<std::string> Graph::getTransitions(std::string from)
{
    std::vector<std::string> v;

    for (Edge *edge : edges_)
    {
       if (edge -> fromName() == from)
       {
           v.push_back(edge -> fullName());
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
        return action_ -> toString();
    }

    return "";
}

std::string Graph::toString()
{
    std::string graph;

    for (Edge *edge : edges_)
    {
        std::vector<std::string> transitions = getTransitions(edge -> toName());

        std::string functionName = edge -> fullName();
        std::string functionAction = edge -> actionToString();
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
