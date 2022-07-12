#include <functional>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>


Compiler::Compiler(Parser& parser) : parser_(parser)
{
    initializeGraph();
}

void Compiler::compile()
{
    generateTypes();
    generateFunctions();
}

void Compiler::initializeGraph()
{
    for (const auto& edge : parser_.getEdges())
    {
        graph_.addEdge(std::make_unique<Edge>(std::make_unique<Node>(edge["lhs"]["parts"]),
            std::make_unique<Node>(edge["rhs"]["parts"]), std::make_unique<Action>(edge["label"])));
    }
}

void Compiler::generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile)
{
    Printer printer(parser_, headerFile, sourceFile);
    printer.initializeHeaderFile();
    printer.printTypeDeclarations(program_.getTypes());
    printer.printSymbolValues();
    printer.printStateChanges(program_.getFunctions());
    // TODO: transform program into C++ source code using printer
}

void Compiler::generateTypes()
{
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] != "Arrow")
        {
            auto newElementaryType = std::make_unique<ElementaryType>();
            newElementaryType->identifier = t["identifier"].get<std::string>();
            program_.addTypeDeclaration(std::move(newElementaryType));
        }
    }
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] == "Arrow")
        {
            auto newFunctionType = generateType(t["type"]);
            newFunctionType->identifier = t["identifier"].get<std::string>();
            program_.addTypeDeclaration(std::move(newFunctionType));
        }
    }
}

void Compiler::generateStateFunctions(std::string type, std::function<std::string(std::string)> begining,
    std::function<std::string(std::string)> innerLoop, std::string ending)
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto &state : states)
    {
        std::vector<std::string> outgoingStates = graph_.getOutgoingNodesFrom(state);

        std::string func;
        func = type + state + "()\n{\n";
        func += begining(state);

        for (auto &outgingState : outgoingStates)
        {
            func += innerLoop(state + "_" + outgingState);
        }

        func += ending;

        program_.addFunction(Function(func));
    }
}

void Compiler::generateEdgeFunctions(std::string type, std::string retVal, std::function<std::string(std::string)> middle,
    std::string ending)
{
    std::vector<std::string> edges = graph_.getEdgeNames();

    for (auto &edge : edges)
    {
        std::string func;

        func += type + edge + "()\n{\n";

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            func += "    int old = " + graph_.getActionLeftSide(edge) + ";\n";
        }
        else if (graph_.getActionType(edge) == ActionType::Comparison)
        {
            func += "    if (" + static_cast<std::string>((graph_.getActionNegationValue(edge) ? "" : "!(")) + graph_.getAction(edge) +
                static_cast<std::string>((graph_.getActionNegationValue(edge) ? "" : ")")) + ")\n";
            func += "    {\n";
            func += "        return " + retVal + ";\n    }\n";
        }
        else if (graph_.getActionType(edge) == ActionType::Reachability)
        {
            func += "    currentPatterns.push_back(\""+ graph_.getActionRightSide(edge) +"\");\n";
            func += "    if (" + static_cast<std::string>((graph_.getActionNegationValue(edge) ? "" : "!")) + "is_legal_" + graph_.getActionLeftSide(edge) + "())\n";
            func += "    {\n";
            func += "        currentPatternNode.pop_back();\n";
            func += "        return "+ retVal +";\n    }\n";
            func += "    currentPatterns.pop_back();\n";
        }
        else if (graph_.getActionType(edge) != ActionType::Skip)
        {
            func += "    " + graph_.getAction(edge) + ";\n";
        }

        func += middle(graph_.getToName(edge));

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            func += "    " + graph_.getActionLeftSide(edge) + " = old;\n";
        }

        func += ending;

        program_.addFunction(Function(func));
    }
}

void Compiler::generateFunctions()
{
    // TODO node names should be represended by numbers not strings
    // TODO this should be changed after the proper implementation of the Function class comes out

    generateStateFunctions("void ", [](std::string str){return "    currentMoves.push_back(\"" + str + "\");\n";},
        [](std::string str){return "    edge_" + str + "();\n";}, "    currentMoves.pop_back();\n}\n");
    generateStateFunctions("bool is_legal_", [](std::string str){return "    if (currentPatterns.back() == \"" + str + "\"){\n        return true\n    }\n";},
        [](std::string str){return "    if (is_legal_edge_" + str + "())\n    { return true; }\n";}, "    return false;\n}\n");

    generateEdgeFunctions("void ", "", [](std::string str){return "    " + str + "();\n";}, "}\n");
    generateEdgeFunctions("bool is_legal_", "false", [](std::string str){return "    bool tmp = is_legal_" + str + "();\n";}, "    return tmp;\n}\n");
}

std::unique_ptr<IType> Compiler::generateType(const nlohmann::json& t)
{
    if (t.is_string())
    {
        return std::make_unique<ElementaryType>(t.get<std::string>());
    }
    else if (t["kind"] == "TypeReference")
    {
        return std::make_unique<ElementaryType>(t["identifier"].get<std::string>());
    }
    else if (t["kind"] == "Arrow")
    {
        return generateFunctionType(t);
    }
    return nullptr;
}

std::unique_ptr<IType> Compiler::generateFunctionType(const nlohmann::json& functionType)
{
    return std::make_unique<FunctionType>(generateType(functionType["lhs"]), generateType(functionType["rhs"]));
}
