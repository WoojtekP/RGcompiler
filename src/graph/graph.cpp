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

    return name_  + bindings + " ";
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

Graph::~Graph()
{
    for (Edge *e : edges_)
    {
        delete e;
    }
}

void Graph::addEdge(Edge *edge)
{
    edges_.emplace_back(edge);
}

void Graph::print()
{
    for (Edge *edge : edges_)
    {
        std::cout << edge -> toString() << "\n";
    }
}
