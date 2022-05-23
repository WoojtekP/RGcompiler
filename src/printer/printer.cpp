#include <iostream>
#include <fstream>

#include <printer/action.hpp>
#include <printer/printer.hpp>

Printer::Printer(const Parser& parser, std::ofstream& headerFile, std::ofstream& sourceFile)
: parser_(parser)
, headerFile_(headerFile)
, sourceFile_(sourceFile)
{
}

void Printer::printHeaderFile()
{
    printIncludes();
    printTypes();
    printConstants();
    printGameState();
}

void Printer::printSourceFile()
{
    printStateChanges(); // Added temporary just to see output
}

void Printer::printIncludes()
{
    headerFile_ << "#include <map>" << std::endl;
    headerFile_ << std::endl;
}

void Printer::printTypes()
{
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] != "Arrow")
        {
            headerFile_ << "using " << t["identifier"].get<std::string>() << " = int;" << std::endl;
        }
    }
    headerFile_ << std::endl;
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] == "Arrow")
        {
            headerFile_ << "using " << t["identifier"].get<std::string>() << " = " << typeToString(t["type"]) << ";" << std::endl;
        }
    }
    headerFile_ << std::endl;
}

std::string Printer::typeToString(const nlohmann::json& t)
{
    if (t.is_string())
    {
        return t;
    }
    else if (t["kind"] == "TypeReference")
    {
        return t["identifier"];
    }
    else if (t["kind"] == "Arrow")
    {
        return functionTypeToString(t);
    }
    return "???";
}

std::string Printer::functionTypeToString(const nlohmann::json& functionType)
{
    return "std::map<" + typeToString(functionType["lhs"]) + ", " + typeToString(functionType["rhs"]) + ">";
}

void Printer::printConstants()
{
    for (const auto& constant : parser_.getConstants())
    {
        const std::string constType = typeToString(constant["type"]);
        const std::string constName = constant["identifier"];
        const std::string constValue =  valueToString(constant["type"], constant["value"]);
        headerFile_ << "const " << constType << " " << constName << " = " << constValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printVariables()
{
    for (const auto& variable : parser_.getVariables())
    {
        const std::string varType = typeToString(variable["type"]);
        const std::string varName = variable["identifier"];
        const std::string varValue =  valueToString(variable["type"], variable["defaultValue"]);
        headerFile_ << varType << " " << varName << " = " << varValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

std::string Printer::valueToString(const nlohmann::json& t, const nlohmann::json& value)
{
    if (value["kind"] == "Element")
    {
        return parser_.getValue(value["identifier"]);
    }
    else if (value["kind"] == "Map")
    {
        std::map<std::string, std::string> identifierToValue;
        const auto destinationType = parser_.getDestinationType(t);
        for (const auto& entry : value["entries"])
        {
            if (entry["kind"] == "NamedEntry")
            {
                identifierToValue.emplace(entry["identifier"], valueToString(destinationType, entry["value"]));
            }
        }
        const auto defaultValue = defaultValueToString(t, value["entries"]);
        const auto sourceType = parser_.getSourceType(t);
        for (const auto& identifier : parser_.getDomain(sourceType))
        {
            if (identifierToValue.find(identifier) == identifierToValue.end())
            {
                identifierToValue.emplace(identifier, defaultValue);
            }
        }
        std::string result = "{";
        int i = 1;
        for (const auto& [id, val] : identifierToValue)
        {
            result += "{" + parser_.getValue(id) + ", " + val + "}";
            if (i < identifierToValue.size())
            {
                result += ", ";
            }
            ++i;
        }
        return result + "}";
    }
    return "?";
}

std::string Printer::defaultValueToString(const nlohmann::json& t, const nlohmann::json& entries)
{
    for (const auto& entry : entries)
    {
        if (entry["kind"] == "DefaultEntry")
        {
            return valueToString(t, entry["value"]);
        }
    }
    return "?";
}

void Printer::printGameState()
{
    headerFile_ << "class Reasoner" << std::endl;
    headerFile_ << "{" << std::endl;
    printVariables();
    headerFile_ << "};" << std::endl;
}

// FIXME: this classes definitions should not be in printer.cpp

class Binding
{
    std::string variableName_;
    std::string iteratedType_;
public:
    Binding(std::string variableName, std::string iteratedType) :
        variableName_(variableName), iteratedType_(iteratedType)
    {}

    std::string toString()
    {
        return "(" + iteratedType_ + ":" + variableName_ + ")";
    }
};

class Node
{
    std::string name_;
    std::vector<Binding> bindings_;
public:
    Node(const nlohmann::json& t)
    {
        name_ = Parser::getValueFromEntries(t, "Literal", "identifier");

        // FIXME: we should parse more than one binding
        const auto& binding = Parser::getPartFromParts(t, "Binding");

        if (binding)
        {
            bindings_.emplace_back((*binding).get()["identifier"], (*binding).get()["type"]["identifier"]);
        }
    }

    std::string toString()
    {
        std::string bindings;

        for (Binding& binding : bindings_)
        {
            bindings += binding.toString();
        }

        return name_  + bindings + " ";
    }
};

class Edge
{
    Node *from_     = nullptr;
    Node *to_       = nullptr;
    Action *action_ = nullptr;

public:
    Edge(Node *from, Node *to, Action *action) :
        from_(from), to_(to), action_(action)
    {
    };

    ~Edge()
    {
        delete from_;
        delete to_;
        delete action_;
    }

    std::string toString()
    {
        return "<" + from_ -> toString() + ", " + to_ -> toString() + ", " + action_ -> toString() + ">";
    }
};
class Graph
{
    std::vector<Edge*> edges_;
public:
    ~Graph()
    {
        for (Edge *e : edges_)
        {
            delete e;
        }
    }

    void addEdge(Edge *edge)
    {
        edges_.emplace_back(edge);
    }

    void print()
    {
        for (Edge *edge : edges_)
        {
            std::cout << edge -> toString() << "\n";
        }
    }
};

void Printer::printStateChanges()
{
    Graph graph;

    for (const auto& edge : parser_.getEdges())
    {
        Node *nodeFrom = new Node(edge["lhs"]["parts"]);
        Node *nodeTo   = new Node(edge["rhs"]["parts"]);
        Action *action = new Action(edge["label"]);

        graph.addEdge(new Edge(nodeFrom, nodeTo, action));
    }

    graph.print();
}
