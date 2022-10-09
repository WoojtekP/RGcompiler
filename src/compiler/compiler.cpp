#include <functional>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>


namespace
{
bool isAnyPairOfEdgesComplementary(const std::vector<std::shared_ptr<Edge>>& edges)
{
    for (const auto& edgeA : edges)
    {
        for (const auto& edgeB : edges)
        {
            if (edgeA != edgeB && edgeA->isComplementaryTo(*edgeB))
            {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Edge> findComplementaryEdge(const std::shared_ptr<Edge>& edge, const std::vector<std::shared_ptr<Edge>>& edges)
{
    for (const auto& outgoingEdge : edges)
    {
        if (edge->isComplementaryTo(*outgoingEdge))
        {
            return outgoingEdge;
        }
    }
    return nullptr;
}
}  // namespace

Compiler::Compiler(Parser& parser, bool debugFlag) : parser_(parser), debugFlag_(debugFlag)
{
    initializeGraph();
    generateIntRepresentationForStates();
    generateIntRepresentationForEdges();
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
        graph_.addEdge(std::make_shared<Edge>(
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
            auto newElementaryType = std::make_shared<ElementaryType>();
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

    auto nameToFunctionType = std::make_shared<CustomType>("std::map<std::string, funcPtr>");
    program_.addVariableDeclaration(std::make_unique<Variable>("nameToFunction", std::move(nameToFunctionType)));

    auto allMovesType = std::make_shared<CustomType>("std::vector<std::vector<int>>");
    program_.addVariableDeclaration(std::make_unique<Variable>("allMoves", std::move(allMovesType)));

    auto currentMovesType = std::make_shared<CustomType>("std::vector<int>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentMoves", std::move(currentMovesType)));

    auto currentPatternsType = std::make_shared<CustomType>("std::vector<int>");
    program_.addVariableDeclaration(std::make_unique<Variable>("currentPatterns", std::move(currentPatternsType)));

    std::string initialState = getStateIntId("begin");

    auto currentStateType = std::make_shared<CustomType>("int");
    auto currentStateValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));

    auto initialType = std::make_shared<CustomType>("static constexpr int");
    auto initialValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("initial", std::move(initialType), std::move(initialValue), true));
}

void Compiler::generateVoidStateFunctions()
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "state_";
        std::string functionName = prefix + getStateIntId(state);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        std::set<std::shared_ptr<Edge>> complementaryEdges;
        const auto& outgoingEdges = graph_.getOutgoingEdgesFrom(state);

        for (auto& outgoingEdge : outgoingEdges)
        {
            if (complementaryEdges.count(outgoingEdge))
            {
                const std::string shouldCheckVarName = "should_check_" + outgoingEdge->toName();
                std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                    std::make_unique<ComparisonInstruction>(shouldCheckVarName, "true"));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>(
                    "edge_" + getStateIntId(state) + "_" + getStateIntId(outgoingEdge->toName()) + "()"));
                function->addInstruction(std::move(ifInstruction));
            }
            else if (const auto complementaryEdge = findComplementaryEdge(outgoingEdge, outgoingEdges))
            {
                complementaryEdges.insert(complementaryEdge);
                const std::string sizeVarName = "size_before_" + outgoingEdge->toName();
                const std::string shouldCheckVarName = "should_check_" + complementaryEdge->toName();
                function->addInstruction(std::make_unique<AssignmentInstruction>(
                    sizeVarName, "allMoves.size()", "const auto"));
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "edge_" + getStateIntId(state) + "_" + getStateIntId(outgoingEdge->toName()) + "()"));
                function->addInstruction(std::make_unique<AssignmentInstruction>(
                    shouldCheckVarName, "(" + sizeVarName + "==allMoves.size())", "const auto"));
            }
            else
            {
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "edge_" + getStateIntId(state) + "_" + getStateIntId(outgoingEdge->toName()) + "()"));
            }
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolStateFunctions()
{
    std::vector<std::string> states = graph_.getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "is_legal_";
        std::string functionName = prefix + getStateIntId(state);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        const auto& outgoingEdges = graph_.getOutgoingEdgesFrom(state);
        if (outgoingEdges.empty() || isAnyPairOfEdgesComplementary(outgoingEdges))
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>("currentPatterns.back()", getStateIntId(state)));

            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));

            function->addInstruction(std::move(ifInstruction));

            for (auto& outgoingEdge : outgoingEdges)
            {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_edge_" + getStateIntId(state) + "_" + getStateIntId(outgoingEdge->toName()) + "()", "true"));
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

    for (const auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "edge_";
        std::string functionName = prefix + getStateIntId(stateFrom) + "_" + getStateIntId(stateTo);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        function->addInstruction(std::make_unique<CustomInstruction>(
            "currentMoves.push_back(" + std::to_string(edgeStringToInt_[std::make_pair(stateFrom, stateTo)]) + ")"));

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            if (graph_.getActionLeftSide(stateFrom, stateTo) == "player")
            {
                function->addInstruction(std::make_unique<CustomInstruction>("allMoves.push_back(currentMoves)"));
                function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));
                function->addInstruction(std::make_unique<ReturnInstruction>());

                program_.addFunction(std::move(function));

                continue;
            }

            function->addInstruction(
                std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(stateFrom, stateTo), "const auto"));
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
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
            function->addInstruction(std::move(ifInstruction));
        }
        else if (graph_.getActionType(stateFrom, stateTo) == ActionType::Reachability)
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "currentPatterns.push_back(" + getStateIntId(graph_.getActionRightSide(stateFrom, stateTo)) + ")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_" + getStateIntId(graph_.getActionLeftSide(stateFrom, stateTo)) + "()",
                    graph_.getActionNegationValue(stateFrom, stateTo) ? "true" : "false"));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
            function->addInstruction(std::move(ifInstruction));
            function->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
        }

        function->addInstruction(std::make_unique<CustomInstruction>("state_" + getStateIntId(stateTo) + "()"));

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(graph_.getActionLeftSide(stateFrom, stateTo), "old"));
        }

        function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolEdgeFunctions()
{
    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (const auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "is_legal_edge_";
        std::string functionName = prefix + getStateIntId(stateFrom) + "_" + getStateIntId(stateTo);
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        if (graph_.getActionType(stateFrom, stateTo) == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>("old", graph_.getActionLeftSide(stateFrom, stateTo), "const auto"));
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
                "currentPatterns.push_back(" + getStateIntId(graph_.getActionRightSide(stateFrom, stateTo)) + ")"));
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_" + getStateIntId(graph_.getActionLeftSide(stateFrom, stateTo)) + "()",
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
                std::make_unique<AssignmentInstruction>("tmp", "is_legal_" + getStateIntId(stateTo) + "()", "bool"));
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

    for (const auto& [stateFrom, stateTo] : edges)
    {
        std::string prefix = "apply_edge_";
        std::string functionName = prefix + getStateIntId(stateFrom) + "_" + getStateIntId(stateTo);
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

void Compiler::generateGetFromStateForEdge()
{
    auto function = std::make_unique<Function>("getFromStateForEdge", "int", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (const auto& [stateFrom, stateTo] : edges)
    {
        sw->addCaseInstruction(
            edgeStringToInt_[std::make_pair(stateFrom, stateTo)],
            std::move(std::make_unique<ReturnInstruction>(getStateIntId(stateTo))));
    }

    function->addInstruction(std::move(sw));
    function->addInstruction(std::make_unique<ReturnInstruction>("-1"));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunApplyEdgeFunction()
{
    auto function = std::make_unique<Function>("runApplyEdge", "void", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (const auto& [stateFrom, stateTo] : edges)
    {
        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(std::make_unique<CustomInstruction>(
            "apply_edge_" + getStateIntId(stateFrom) + "_" + getStateIntId(stateTo) + "()"));
        block->addInstruction(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(edgeStringToInt_[std::make_pair(stateFrom, stateTo)], std::move(block));
    }

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunStateFunction()
{
    auto function = std::make_unique<Function>("runState", "void", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    for (auto& state : graph_.getNodeNames())
    {
        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(std::make_unique<CustomInstruction>("state_" + getStateIntId(state) + "()"));
        block->addInstruction(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(stateStringToInt_[state], std::move(block));
    }

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateSpecialFunctions()
{
    generateRunApplyEdgeFunction();
    generateRunStateFunction();
    generateGetFromStateForEdge();

    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", true);
    isTerminal->addInstruction(std::make_unique<ReturnInstruction>("currentState == " + getStateIntId("end")));

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
    runState(currentState);
    moves.assign(allMoves.begin(), allMoves.end()))"));

    auto applyMoveFunction = std::make_unique<Function>("applyMove", "void", true);
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("m", "const Move&"));
    applyMoveFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(const std::vector<int> &v = m.mr;

    for (int edge : v)
    {
       runApplyEdge(edge);
    }

    currentState = getFromStateForEdge(v.back()))"));

    program_.addFunction(std::move(isTerminal));
    program_.addFunction(std::move(getPlayerScore));
    program_.addFunction(std::move(getCurrentPlayer));
    program_.addFunction(std::move(getCurrentState));
    program_.addFunction(std::move(getAllMovesFunction));
    program_.addFunction(std::move(applyMoveFunction));
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

std::shared_ptr<IType> Compiler::generateType(const nlohmann::json& t)
{
    if (t.is_string())
    {
        return std::make_shared<ElementaryType>(t.get<std::string>());
    }
    else if (t["kind"] == "TypeReference")
    {
        return program_.findType(t["identifier"].get<std::string>());
    }
    else if (t["kind"] == "Arrow")
    {
        return generateFunctionType(t);
    }
    return nullptr;
}

std::shared_ptr<IType> Compiler::generateFunctionType(const nlohmann::json& functionType)
{
    auto sourceType = generateType(functionType["lhs"]);
    auto destinationType = generateType(functionType["rhs"]);
    const std::string sourceTypeName = sourceType->identifier;
    const auto& symbolToValueMap = parser_.getTypeToSymbolToValueMap().at(sourceTypeName);
    const auto maxDomainValueIt = std::max_element(
        symbolToValueMap.begin(),
        symbolToValueMap.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
    return std::make_shared<FunctionType>(std::move(sourceType), std::move(destinationType), maxDomainValueIt->second + 1);
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

void Compiler::generateIntRepresentationForStates()
{
    for (auto& state : graph_.getNodeNames())
    {
        stateStringToInt_[state] = stateStringToInt_.size();
    }
}

void Compiler::generateIntRepresentationForEdges()
{
    std::vector<std::pair<std::string, std::string>> edges = graph_.getEdgeNames();

    int shift = stateStringToInt_.size();

    for (const auto& [stateFrom, stateTo] : edges)
    {
        edgeStringToInt_[std::make_pair(stateFrom, stateTo)] = edgeStringToInt_.size() + shift;
    }
}

std::string Compiler::getStateIntId(std::string name)
{
    if (stateStringToInt_.count(name) > 0)
    {
        return std::to_string(stateStringToInt_[name]);
    }

    return "";
}
