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
    generateConstants();
    generateVariables();
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
    printer.initializeSourceFile();
    printer.printTypeDeclarations(program_.getTypes());
    printer.printSymbolValues();
    printer.printConstants(program_.getConstants());
    printer.printVariables(program_.getVariables());
    printer.printStateChanges(program_.getFunctions());
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

void Compiler::generateConstants()
{
    for (const auto& constant : parser_.getConstants())
    {
        auto valueType = generateType(constant["type"]);
        auto value = generateValue(constant["value"]);
        const std::string identifier = constant["identifier"].get<std::string>();
        program_.addConstantDeclaration(
            std::make_unique<Constant>(identifier, std::move(valueType), std::move(value)));
    }
}

void Compiler::generateVariables()
{
    for (const auto& variable : parser_.getVariables())
    {
        auto valueType = generateType(variable["type"]);
        auto value = generateValue(variable["defaultValue"]);
        const std::string identifier = variable["identifier"].get<std::string>();
        program_.addVariableDeclaration(
            std::make_unique<Variable>(identifier, std::move(valueType), std::move(value)));
    }

    auto currentMovesType = std::make_unique<CustomType>("std::vector<std::string>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentMoves", std::move(currentMovesType)));

    auto currentPatternsType = std::make_unique<CustomType>("std::vector<std::string>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentPatterns", std::move(currentPatternsType)));

    auto currentStateType = std::make_unique<CustomType>("int");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentState", std::move(currentStateType)));
}

void Compiler::generateVoidStateFunctions()
{

    std::vector<std::string> states = graph_.getNodeNames();

    for (auto &state : states)
    {
        std::unique_ptr<Function> function = std::make_unique<Function>(state, "void");

        function -> addInstruction(std::make_unique<CustomInstruction>("currentMoves.push_back(\"" + state + "\")"));

        std::vector<std::string> outgoingStates = graph_.getOutgoingNodesFrom(state);

        for (auto &outgingState : outgoingStates)
        {
            function -> addInstruction(std::make_unique<CustomInstruction>("edge_" + state + "_" + outgingState + "()"));
        }

        function -> addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolStateFunctions()
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto &state : states)
    {
        std::unique_ptr<Function> function = std::make_unique<Function>("is_legal_" + state, "bool");

        std::unique_ptr<IfInstruction> ifInstruction =
            std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>("currentPatterns.back()", "\"" + state + "\""));

        ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>("true"));

        function -> addInstruction(std::move(ifInstruction));

        std::vector<std::string> outgoingStates = graph_.getOutgoingNodesFrom(state);

        for (auto &outgingState : outgoingStates)
        {
            ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>("is_legal_edge_" + state + "_" + outgingState + "()", "true"));
            ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>("true"));
            function -> addInstruction(std::move(ifInstruction));
        }

        function -> addInstruction(std::make_unique<ReturnInstruction>("false"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateVoidEdgeFunctions()
{
    std::vector<std::string> edges = graph_.getEdgeNames();

    for (auto &edge : edges)
    {
        std::unique_ptr<Function> function = std::make_unique<Function>(edge, "void");

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            function -> addInstruction(std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(edge), "int"));
            function -> addInstruction(std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(edge), graph_.getActionRightSide(edge)));
        }
        else if (graph_.getActionType(edge) == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(graph_.getActionLeftSide(edge),
                graph_.getActionRightSide(edge), !graph_.getActionNegationValue(edge)));
            ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>());
            function -> addInstruction(std::move(ifInstruction));
        }
        else if (graph_.getActionType(edge) == ActionType::Reachability)
        {
            function -> addInstruction(std::make_unique<CustomInstruction>("currentPatterns.push_back(\"" + graph_.getActionRightSide(edge) + "\")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>("is_legal_" + graph_.getActionLeftSide(edge) + "()",
                "false"));
            ifInstruction -> addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>());
            function -> addInstruction(std::move(ifInstruction));
        }

        function -> addInstruction(std::make_unique<CustomInstruction>(graph_.getToName(edge) + "()"));

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            function -> addInstruction(std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(edge), "old"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolEdgeFunctions()
{
    std::vector<std::string> edges = graph_.getEdgeNames();

    for (auto &edge : edges)
    {
        std::unique_ptr<Function> function = std::make_unique<Function>("is_legal_" + edge, "bool");

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            function -> addInstruction(std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(edge), "int"));
            function -> addInstruction(std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(edge), graph_.getActionRightSide(edge)));
        }
        else if (graph_.getActionType(edge) == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(graph_.getActionLeftSide(edge),
                graph_.getActionRightSide(edge), !graph_.getActionNegationValue(edge)));
            ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>("false"));
            function -> addInstruction(std::move(ifInstruction));
        }
        else if (graph_.getActionType(edge) == ActionType::Reachability)
        {
            function -> addInstruction(std::make_unique<CustomInstruction>("currentPatterns.push_back(\"" + graph_.getActionRightSide(edge) + "\")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>("is_legal_" + graph_.getActionLeftSide(edge),
                "false"));
            ifInstruction -> addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            ifInstruction -> addInstruction(std::make_unique<ReturnInstruction>("false"));
            function -> addInstruction(std::move(ifInstruction));
        }

        function -> addInstruction(std::make_unique<AssignmentInstruction>("tmp", "is_legal_" + graph_.getToName(edge) + "()", "bool"));

        if (graph_.getActionType(edge) == ActionType::Assignment)
        {
            function -> addInstruction(std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(edge), "old"));
        }

        function -> addInstruction(std::make_unique<ReturnInstruction>("tmp"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateFunctions()
{
    // TODO node names should be represended by numbers not strings
    // TODO this should be changed after the proper implementation of the Function class comes out

    generateVoidStateFunctions();
    generateBoolStateFunctions();

    generateBoolEdgeFunctions();
    generateVoidEdgeFunctions();
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

std::unique_ptr<IValue> Compiler::generateValue(const nlohmann::json& value)
{
    if (value["kind"] == "Element")
    {
        return std::make_unique<SingleValue>(value["identifier"].get<std::string>());
    }
    else if (value["kind"] == "Map")
    {
        return generateMapValue(value);
    }
    return nullptr;
}

std::unique_ptr<IValue> Compiler::generateMapValue(const nlohmann::json& value)
{
    std::map<std::string, std::unique_ptr<IValue>> idToValueMap;
    std::unique_ptr<IValue> defaultValue;
    for (const auto& entry : value["entries"])
    {
        if (entry["kind"] == "NamedEntry")
        {
            idToValueMap.emplace(entry["identifier"].get<std::string>(), generateValue(entry["value"]));
        }
        else if (entry["kind"] == "DefaultEntry")
        {
            defaultValue = generateValue(entry["value"]);
        }
    }
    return std::make_unique<MapValue>(std::move(idToValueMap), std::move(defaultValue));
}
