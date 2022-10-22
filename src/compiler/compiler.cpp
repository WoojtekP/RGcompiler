#include <functional>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>

namespace
{
bool isAnyPairOfEdgesComplementary(const std::vector<std::pair<std::shared_ptr<Edge>, int>>& edges)
{
    for (const auto& [edgeA, id] : edges)
    {
        for (const auto& [edgeB, id] : edges)
        {
            if (edgeA != edgeB && edgeA->isComplementaryTo(*edgeB))
            {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Edge> findComplementaryEdge(
    const std::shared_ptr<Edge>& edge, const std::vector<std::pair<std::shared_ptr<Edge>, int>>& edges)
{
    for (const auto& [outgoingEdge, id] : edges)
    {
        if (edge->isComplementaryTo(*outgoingEdge))
        {
            return outgoingEdge;
        }
    }
    return nullptr;
}
}  // namespace

Compiler::Compiler(Parser& parser, const Options& options)
: parser_(parser), debugFlag_(options.debug),
  optConditionsReachability_(options.optConditions == 1 || options.optConditions == 3),
  optConditionsGeneratingMoves_(options.optConditions == 2 || options.optConditions == 3),
  optConditionsSimplePaths_((options.optConditions == 4 || options.optConditions == 3)),
  temporaryVariableNamePrefix_("old")
{
    initializeGraph();
}

void Compiler::compile()
{
    generateTypes();
    generateConstants();
    generateVariables(graph_);
    generateFunctions();
}

void Compiler::initializeGraph()
{
    graph_ = std::make_shared<Graph>();

    for (const auto& edge : parser_.getEdges())
    {
        graph_->addEdge(std::make_shared<Edge>(
            std::make_shared<Node>(edge["lhs"]["parts"]),
            std::make_shared<Node>(edge["rhs"]["parts"]),
            std::vector<std::shared_ptr<Action>> {std::make_shared<Action>(edge["label"])}));
    }

    graph_->initialize();

    if (optConditionsSimplePaths_)
    {
        graph_ = graph_->getGraphWithOptimizedPaths();
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

void Compiler::generateVariables(const std::shared_ptr<Graph>& graph)
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

    std::string initialState = std::to_string(graph->getNodeId("begin"));

    auto currentStateType = std::make_shared<CustomType>("int");
    auto currentStateValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));

    auto initialType = std::make_shared<CustomType>("static constexpr int");
    auto initialValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("initial", std::move(initialType), std::move(initialValue), true));
}

template<typename T>
void Compiler::restoreAssignments(const std::unique_ptr<T>& function, std::vector<std::shared_ptr<Action>> assignments)
{
    int cnt = assignments.size() - 1;
    std::reverse(assignments.begin(), assignments.end());
    for (const auto& action : assignments)
    {
        function->addInstruction(std::make_unique<AssignmentInstruction>(
            action->getLeftSide(), std::string(temporaryVariableNamePrefix_ + std::to_string(cnt))));
        cnt--;
    }
}

void Compiler::generateVoidStateFunctions(const std::shared_ptr<Graph>& graph)
{
    std::vector<std::string> states = graph->getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "state_";
        std::string functionName = prefix + std::to_string(graph->getNodeId(state));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        if (optConditionsGeneratingMoves_)
        {
            generateVoidStateOptimizedFunction(state, function, graph);
        }
        else
        {
            for (auto [outgoingEdge, id] : graph->getOutgoingEdgesFrom(state))
            {
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), id)) + "()"));
            }
        }
        program_.addFunction(std::move(function));
    }
}

void Compiler::generateVoidStateOptimizedFunction(
    const std::string& state, const std::unique_ptr<Function>& function, const std::shared_ptr<Graph>& graph)
{
    std::set<std::shared_ptr<Edge>> complementaryEdges;
    const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

    for (auto [outgoingEdge, id] : outgoingEdges)
    {
        if (complementaryEdges.count(outgoingEdge))
        {
            const std::string shouldCheckVarName = "should_check_" + outgoingEdge->toName();
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(shouldCheckVarName, "true"));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), id)) + "()"));
            function->addInstruction(std::move(ifInstruction));
        }
        else if (const auto complementaryEdge = findComplementaryEdge(outgoingEdge, outgoingEdges))
        {
            complementaryEdges.insert(complementaryEdge);
            const std::string sizeVarName = "size_before_" + outgoingEdge->toName();
            const std::string shouldCheckVarName = "should_check_" + complementaryEdge->toName();
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(sizeVarName, "allMoves.size()", "const auto"));
            function->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), id)) + "()"));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                shouldCheckVarName, "(" + sizeVarName + "==allMoves.size())", "const auto"));
        }
        else
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), id)) + "()"));
        }
    }
}

void Compiler::generateBoolStateFunctions(const std::shared_ptr<Graph>& graph)
{
    std::vector<std::string> states = graph->getNodeNames();

    for (auto& state : states)
    {
        std::string prefix = "is_legal_";
        std::string functionName = prefix + std::to_string(graph->getNodeId(state));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

        if (outgoingEdges.empty() || (optConditionsReachability_ && isAnyPairOfEdgesComplementary(outgoingEdges)))
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "currentPatterns.back()", std::to_string(graph->getNodeId(state))));

            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));

            function->addInstruction(std::move(ifInstruction));

            for (auto& [outgoingEdge, id] : outgoingEdges)
            {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), id)) + "()",
                    "true"));
                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
                function->addInstruction(std::move(ifInstruction));
            }

            function->addInstruction(std::make_unique<ReturnInstruction>("false"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateVoidEdgeFunctions(const std::shared_ptr<Graph>& graph)
{
    const auto& edges = graph->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        std::string prefix = "edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        const auto& actions = graph->getActions(stateFrom, stateTo, edgeId);
        std::vector<std::shared_ptr<Action>> assignmentActions;

        function->addInstruction(std::make_unique<CustomInstruction>(
            "currentMoves.push_back(" + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId)) + ")"));

        bool playerChanged = false;

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment)
            {
                if (action->getLeftSide() == "player")
                {
                    function->addInstruction(std::make_unique<CustomInstruction>("allMoves.push_back(currentMoves)"));
                    function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));
                    restoreAssignments<Function>(function, assignmentActions);
                    function->addInstruction(std::make_unique<ReturnInstruction>());
                    program_.addFunction(std::move(function));
                    playerChanged = true;
                    break;
                }
                function->addInstruction(std::make_unique<AssignmentInstruction>(
                    std::string(temporaryVariableNamePrefix_ + std::to_string(assignmentActions.size())),
                    action->getLeftSide(),
                    "const auto"));
                function->addInstruction(
                    std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
                assignmentActions.push_back(action);
            }
            else if (action->getType() == ActionType::Comparison)
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        action->getLeftSide(), action->getRightSide(), !action->getNegated()));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
                function->addInstruction(std::move(ifInstruction));
            }
            else if (action->getType() == ActionType::Reachability)
            {
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "currentPatterns.push_back(" + std::to_string(graph->getNodeId(action->getRightSide())) + ")"));
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "is_legal_" + std::to_string(graph->getNodeId(action->getLeftSide())) + "()",
                        action->getNegated() ? "true" : "false"));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
                function->addInstruction(std::move(ifInstruction));
                function->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            }
        }

        if (playerChanged)
        {
            continue;
        }

        function->addInstruction(
            std::make_unique<CustomInstruction>("state_" + std::to_string(graph->getNodeId(stateTo)) + "()"));

        restoreAssignments<Function>(function, assignmentActions);

        function->addInstruction(std::make_unique<CustomInstruction>("currentMoves.pop_back()"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolEdgeFunctions(const std::shared_ptr<Graph>& graph)
{
    const auto& edges = graph->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        const auto& actions = graph->getActions(stateFrom, stateTo, edgeId);
        std::vector<std::shared_ptr<Action>> assignmentActions;

        std::string prefix = "is_legal_edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        const auto& innerNodes = graph->getEdge(stateFrom, stateTo, edgeId)->getInnerNodes();

        int numberOfActions = 0;

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment)
            {
                function->addInstruction(std::make_unique<AssignmentInstruction>(
                    temporaryVariableNamePrefix_ + std::to_string(assignmentActions.size()),
                    action->getLeftSide(),
                    "const auto"));
                function->addInstruction(
                    std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
                assignmentActions.push_back(action);
            }
            else if (action->getType() == ActionType::Comparison)
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        action->getLeftSide(), action->getRightSide(), !action->getNegated()));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
                function->addInstruction(std::move(ifInstruction));
            }
            else if (action->getType() == ActionType::Reachability)
            {
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "currentPatterns.push_back(" + std::to_string(graph->getNodeId(action->getRightSide())) + ")"));
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "is_legal_" + std::to_string(graph->getNodeId(action->getLeftSide())) + "()",
                        action->getNegated() ? "true" : "false"));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
                function->addInstruction(std::move(ifInstruction));
                function->addInstruction(std::make_unique<CustomInstruction>("currentPatterns.pop_back()"));
            }
            if (numberOfActions > 0 && numberOfActions < actions.size() - 1)
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "currentPatterns.back()",
                        std::to_string(graph->getNodeId(innerNodes[numberOfActions - 1]->getName()))));
                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);
                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
                function->addInstruction(std::move(ifInstruction));
            }
            numberOfActions++;
        }

        bool outNodesExist = !graph->getOutgoingNodesFrom(stateTo).empty();

        if (outNodesExist)
        {
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                "tmp", "is_legal_" + std::to_string(graph->getNodeId(stateTo)) + "()", "bool"));
        }

        restoreAssignments<Function>(function, assignmentActions);

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

void Compiler::generateApplyEdgeFunctions(const std::shared_ptr<Graph>& graph)
{
    const auto& edges = graph->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        std::string prefix = "apply_edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");
        const auto& actions = graph->getActions(stateFrom, stateTo, edgeId);

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment)
            {
                function->addInstruction(
                    std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
            }
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateGetFromStateForEdge(const std::shared_ptr<Graph>& graph)
{
    auto function = std::make_unique<Function>("getFromStateForEdge", "int", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    const auto& edges = graph->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        sw->addCaseInstruction(
            graph->getEdgeId(stateFrom, stateTo, edgeId),
            std::move(std::make_unique<ReturnInstruction>(std::to_string(graph->getNodeId(stateTo)))));
    }

    function->addInstruction(std::move(sw));
    function->addInstruction(std::make_unique<ReturnInstruction>("-1"));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunApplyEdgeFunction(const std::shared_ptr<Graph>& graph)
{
    auto function = std::make_unique<Function>("runApplyEdge", "void", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    const auto& edges = graph->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(std::make_unique<CustomInstruction>(
            "apply_edge_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId)) + "()"));
        block->addInstruction(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(block));
    }

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunStateFunction(const std::shared_ptr<Graph>& graph)
{
    auto function = std::make_unique<Function>("runState", "void", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (auto& state : graph->getNodeNames())
    {
        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(
            std::make_unique<CustomInstruction>("state_" + std::to_string(graph->getNodeId(state)) + "()"));
        block->addInstruction(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getNodeId(state), std::move(block));
    }

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateSpecialFunctions(const std::shared_ptr<Graph>& graph)
{
    generateRunApplyEdgeFunction(graph);
    generateRunStateFunction(graph);
    generateGetFromStateForEdge(graph);

    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", true);
    isTerminal->addInstruction(
        std::make_unique<ReturnInstruction>("currentState == " + std::to_string(graph->getNodeId("end"))));

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

    generateApplyEdgeFunctions(graph_);

    generateVoidStateFunctions(graph_);
    generateBoolStateFunctions(graph_);

    generateBoolEdgeFunctions(graph_);
    generateVoidEdgeFunctions(graph_);

    generateSpecialFunctions(graph_);
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
    const auto maxDomainValueIt =
        std::max_element(symbolToValueMap.begin(), symbolToValueMap.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second < rhs.second;
        });
    return std::make_shared<FunctionType>(
        std::move(sourceType), std::move(destinationType), maxDomainValueIt->second + 1);
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
