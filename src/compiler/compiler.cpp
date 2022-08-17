#include <functional>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>

Compiler::Compiler(Parser& parser, bool debugFlag) : parser_(parser), debugFlag_(debugFlag)
{
    initializeGraph();
}

void Compiler::compile()
{
    changeStateNamesFromStringToInt();

    generateTypes();
    generateConstants();
    generateVariables();
    generateFunctions();
}

void Compiler::initializeGraph()
{
    for (const auto& edge : parser_.getEdges())
    {
        graph_.addEdge(std::make_unique<Edge>(
            std::make_unique<Node>(edge["lhs"]["parts"]),
            std::make_unique<Node>(edge["rhs"]["parts"]),
            std::make_unique<Action>(edge["label"])));
    }
}

void Compiler::generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile)
{
    Printer printer(parser_, headerFile, sourceFile);
    printer.initializeHeaderFile(debugFlag_);
    printer.initializeSourceFile();
    printer.printTypeDeclarations(program_.getTypes());
    printer.printSymbolValues();
    printer.printConstants(program_.getConstants());
    printer.printMoveRepresentationDeclaration();
    printer.initializeMainClass();
    printer.printVariables(program_.getVariables());
    printer.printFunctions(program_.getFunctions());
    printer.endMainClass();
    printer.endHeaderFile();
    printer.endSourceFile();
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
        program_.addConstantDeclaration(std::make_unique<Constant>(identifier, std::move(valueType), std::move(value)));
    }
}

void Compiler::generateVariables()
{
    for (const auto& variable : parser_.getVariables())
    {
        auto valueType = generateType(variable["type"]);
        auto value = generateValue(variable["defaultValue"]);
        const std::string identifier = variable["identifier"].get<std::string>();
        program_.addVariableDeclaration(std::make_unique<Variable>(identifier, std::move(valueType), std::move(value)));
    }

    auto nameToFunctionType = std::make_unique<CustomType>("std::map<std::string, funcPtr>");
    program_.addVariableDeclaration(std::make_unique<Variable>("nameToFunction", std::move(nameToFunctionType)));

    auto allMovesType = std::make_unique<CustomType>("std::vector<std::vector<int>>");
    program_.addVariableDeclaration(std::make_unique<Variable>("allMoves", std::move(allMovesType)));

    auto currentMovesType = std::make_unique<CustomType>("std::vector<int>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentMoves", std::move(currentMovesType)));

    auto currentPatternsType = std::make_unique<CustomType>("std::vector<int>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentPatterns", std::move(currentPatternsType)));

    auto currentStateType = std::make_unique<CustomType>("int");
    auto currentStateValue = std::make_unique<SingleValue>(getStateName("begin"));
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));
}

void Compiler::generateVoidStateFunctions()
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "state_";
        std::string functionName = prefix + getStateName(state);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");
        functionNameToState_[functionName] = state;

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        function->addInstruction(
            std::make_unique<CustomInstruction>("currentMoves.push_back(" + getStateName(state) + ")"));

        std::vector<std::string> outgoingStates = graph_.getOutgoingNodesFrom(state);

        for (auto& outgingState : outgoingStates)
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + getStateName(state) + "_" + getStateName(outgingState) + "()"));
        }

        function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolStateFunctions()
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "is_legal_";
        std::string functionName = prefix + getStateName(state);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
        functionNameToState_[functionName] = state;

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        std::vector<std::string> outgoingStates = graph_.getOutgoingNodesFrom(state);

        if (outgoingStates.empty())
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>("currentPatterns.back()", getStateName(state)));

            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));

            function->addInstruction(std::move(ifInstruction));

            for (auto& outgingState : outgoingStates)
            {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_edge_" + getStateName(state) + "_" + getStateName(outgingState) + "()", "true"));
                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
                function->addInstruction(std::move(ifInstruction));
            }

            function->addInstruction(std::make_unique<ReturnInstruction>("false"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateVoidEdgeFunctions()
{
    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "edge_";
        std::string functionName = prefix + getStateName(stateFrom) + "_" + getStateName(stateTo);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            if (graph_.getActionLeftSide(stateFrom, stateTo) == "player")
            {
                function->addInstruction(
                    std::make_unique<CustomInstruction>("currentMoves.push_back(" + getStateName(stateTo) + ")"));
                function->addInstruction(std::make_unique<CustomInstruction>("allMoves.push_back(currentMoves)"));
                function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));
                function->addInstruction(std::make_unique<ReturnInstruction>());

                program_.addFunction(std::move(function));

                continue;
            }

            function->addInstruction(
                std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(stateFrom, stateTo), "int"));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                graph_.getActionLeftSide(stateFrom, stateTo), graph_.getActionRightSide(stateFrom, stateTo)));
        }
        else if (graph_.getActionType(stateFrom, stateTo) == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    graph_.getActionLeftSide(stateFrom, stateTo),
                    graph_.getActionRightSide(stateFrom, stateTo),
                    !graph_.getActionNegationValue(stateFrom, stateTo)));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
            function->addInstruction(std::move(ifInstruction));
        }
        else if (graph_.getActionType(stateFrom, stateTo) == ActionType::Reachability)
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "currentPatterns.push_back(" + getStateName(graph_.getActionRightSide(stateFrom, stateTo)) + ")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_" + getStateName(graph_.getActionLeftSide(stateFrom, stateTo)) + "()",
                    graph_.getActionNegationValue(stateFrom, stateTo) ? "true" : "false"));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
            function->addInstruction(std::move(ifInstruction));
            function->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
        }

        function->addInstruction(std::make_unique<CustomInstruction>("state_" + getStateName(stateTo) + "()"));

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(stateFrom, stateTo), "old"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolEdgeFunctions()
{
    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "is_legal_edge_";
        std::string functionName = prefix + getStateName(stateFrom) + "_" + getStateName(stateTo);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(stateFrom, stateTo), "int"));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                graph_.getActionLeftSide(stateFrom, stateTo), graph_.getActionRightSide(stateFrom, stateTo)));
        }
        else if (graph_.getActionType(stateFrom, stateTo) == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    graph_.getActionLeftSide(stateFrom, stateTo),
                    graph_.getActionRightSide(stateFrom, stateTo),
                    !graph_.getActionNegationValue(stateFrom, stateTo)));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
            function->addInstruction(std::move(ifInstruction));
        }
        else if (graph_.getActionType(stateFrom, stateTo) == ActionType::Reachability)
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "currentPatterns.push_back(" + getStateName(graph_.getActionRightSide(stateFrom, stateTo)) + ")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_" + getStateName(graph_.getActionLeftSide(stateFrom, stateTo)) + "()",
                    graph_.getActionNegationValue(stateFrom, stateTo) ? "true" : "false"));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
            function->addInstruction(std::move(ifInstruction));
            function->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
        }

        bool outNodesExist = !graph_.getOutgoingNodesFrom(stateTo).empty();

        if (outNodesExist)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>("tmp", "is_legal_" + getStateName(stateTo) + "()", "bool"));
        }

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(stateFrom, stateTo), "old"));
        }

        if (outNodesExist)
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("tmp"));
        }
        else
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateApplyEdgeFunctions()
{
    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "apply_edge_";
        std::string functionName = prefix + getStateName(stateFrom) + "_" + getStateName(stateTo);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                graph_.getActionLeftSide(stateFrom, stateTo), graph_.getActionRightSide(stateFrom, stateTo)));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateSpecialFunctions()
{
    std::unique_ptr<Function> gameStateConstructor = std::make_unique<Function>("GameState", "", true);

    for (const auto& name : program_.getFunctionNames("void"))
    {
        std::string key = functionNameToState_.count(name) > 0 ? getStateName(functionNameToState_[name]) : name;
        std::string instruction = "nameToFunction[\"" + key + "\"] = &GameState::" + name;
        gameStateConstructor->addInstruction(std::make_unique<CustomInstruction>(instruction));
    }

    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", true);
    isTerminal->addInstruction(std::make_unique<ReturnInstruction>("currentState == " + getStateName("end")));

    auto getPlayerScore = std::make_unique<Function>("getPlayerScore", "Score", true);
    getPlayerScore->addArgument(std::make_unique<VariableDeclarationInstruction>("player", "Player"));
    getPlayerScore->addInstruction(std::make_unique<ReturnInstruction>("goals[player]"));

    auto getCurrentPlayer = std::make_unique<Function>("getCurrentPlayer", "PlayerOrKeeper", true);
    getCurrentPlayer->addInstruction(std::make_unique<ReturnInstruction>("player"));

    auto getCurrentState = std::make_unique<Function>("getCurrentState", "std::string", true);
    getCurrentState->addInstruction(std::make_unique<ReturnInstruction>("std::to_string(currentState)"));

    auto getAllMovesFunction = std::make_unique<Function>("getAllMoves", "void", true);
    getAllMovesFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
    getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(allMoves.clear();
    moves.clear();
    runFunction(std::to_string(currentState));
    moves.assign(allMoves.begin(), allMoves.end()))"));

    auto applyMoveFunction = std::make_unique<Function>("applyMove", "void", true);
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("m", "const Move&"));
    applyMoveFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(const std::vector<int> &v = m.mr;
    int from = v.front();
    int to;
    for (int i=1;i<v.size();i++)
    {
        to = v[i];
        runFunction("apply_edge_" + std::to_string(from) + "_" + std::to_string(to));
        from = to;
    }
    currentState = v.back())"));

    auto runFunction = std::make_unique<Function>("runFunction", "void");
    runFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("functionName", "std::string"));
    runFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(if (nameToFunction.count(functionName) > 0)
        (this->*nameToFunction.at(functionName))())"));

    program_.addFunction(std::move(gameStateConstructor));
    program_.addFunction(std::move(isTerminal));
    program_.addFunction(std::move(getPlayerScore));
    program_.addFunction(std::move(getCurrentPlayer));
    program_.addFunction(std::move(getCurrentState));
    program_.addFunction(std::move(getAllMovesFunction));
    program_.addFunction(std::move(applyMoveFunction));
    program_.addFunction(std::move(runFunction));
}

void Compiler::generateFunctions()
{
    // TODO node names should be represended by numbers not strings
    // TODO this should be changed after the proper implementation of the Function class comes out

    generateApplyEdgeFunctions();

    generateVoidStateFunctions();
    generateBoolStateFunctions();

    generateBoolEdgeFunctions();
    generateVoidEdgeFunctions();

    generateSpecialFunctions();
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

std::unique_ptr<IInstruction> Compiler::debugInstruction(std::string functionName)
{
    std::string information = "In function: " + functionName + "\\n";
    return std::make_unique<CustomInstruction>("std::cout << \"" + information + "\"");
}

void Compiler::changeStateNamesFromStringToInt()
{
    for (auto& state : graph_.getNodeNames())
    {
        statesStringToInt_[state] = statesStringToInt_.size();
    }
}

std::string Compiler::getStateName(std::string name)
{
    if (statesStringToInt_.count(name) > 0)
    {
        return std::to_string(statesStringToInt_[name]);
    }

    return "";
}