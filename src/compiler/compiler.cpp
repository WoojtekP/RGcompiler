#include <functional>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>

namespace
{
bool isAnyPairOfEdgesComplementary(const std::vector<std::pair<std::shared_ptr<Edge>, int>>& edges)
{
    for (const auto& [edgeA, iid] : edges)
    {
        for (const auto& [edgeB, iid] : edges)
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
    for (const auto& [outgoingEdge, iid] : edges)
    {
        if (edge->isComplementaryTo(*outgoingEdge))
        {
            return outgoingEdge;
        }
    }
    return nullptr;
}
}  // namespace

Compiler::Compiler(const Parser& parser, const Options& options)
: parser_(parser), valueAssigner_(parser_.getTypeDeclarations()), debugFlag_(options.debug),
  optConditionsReachability_(options.optConditions == 1 || options.optConditions == 3),
  optConditionsGeneratingMoves_(options.optConditions == 2 || options.optConditions == 3),
  optConditionsSimplePathCompression_(options.simplePathCompression_),
  optConditionsMoveCompression_(options.moveCompression_), temporaryVariableNamePrefix_("old")
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
    ActionFactory actionFactory(parser_, valueAssigner_);

    graph_ = std::make_shared<Graph>();

    for (const auto& edge : parser_.getEdges())
    {
        graph_->addEdge(std::make_shared<Edge>(
            std::make_shared<Node>(edge["lhs"]["parts"]),
            std::make_shared<Node>(edge["rhs"]["parts"]),
            std::vector<std::shared_ptr<IAction>> {actionFactory.createAction(edge["label"])}));
    }

    graph_->initialize();

    patternReachabilityGraphs_ = graph_->generateGraphForPatterns(ActionType::Reachability);
    patternAnyGraphs_ = graph_->generateGraphForPatterns(ActionType::PatternAny);
    applyAnyMoveGraphs_ = graph_->generateGraphsForApplyAnyMove();

    if (optConditionsSimplePathCompression_)
    {
        graph_ = graph_->getGraphWithOptimizedPaths();
    }

    initializePatternGraphs(patternReachabilityGraphs_, 0);
    initializePatternGraphs(patternAnyGraphs_, 1);
    initializePatternGraphs(applyAnyMoveGraphs_, 2);
}

void Compiler::initializePatternGraphs(
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& patterns, int patternId)
{
    for (const auto& [from, to, graph] : patterns)
    {
        graph->initialize();
    }

    if (optConditionsSimplePathCompression_)
    {
        for (size_t i = 0; i < patterns.size(); i++)
        {
            auto& graph = std::get<2>(patterns[i]);

            patterns[i] = std::make_tuple(
                std::get<0>(patterns[i]), std::get<1>(patterns[i]), graph->getGraphWithOptimizedPaths());
        }
    }

    for (const auto& [from, to, graph] : patterns)
    {
        variablesInPatternGraphs_[std::make_tuple(from, to, patternId)] = std::map<std::string, int>();
        graph->getVariablesInPatternGraphs(variablesInPatternGraphs_.at({from, to, patternId}));
        containerChooser_.add({from, to, patternId}, variablesInPatternGraphs_.at({from, to, patternId}));
    }
}

void Compiler::generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile)
{
    Printer printer(parser_, valueAssigner_, headerFile, sourceFile);
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

    auto playerCountConstantType = std::make_shared<CustomType>("int");
    auto playerCountConstantValue = std::make_unique<SingleValue>(std::to_string(getNumberOfPlayers()));
    const std::string playerCountConstantName = "PLAYERS_COUNT";
    program_.addConstantDeclaration(std::make_unique<Constant>(
        playerCountConstantName, std::move(playerCountConstantType), std::move(playerCountConstantValue)));
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

    std::string initialState = std::to_string(graph->getNodeId("begin"));

    auto currentStateType = std::make_shared<CustomType>("int");
    auto currentStateValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));

    // TODO: this is too tricky (declaring variable with type using), need proper implementation
    for (const auto& [type, customType] : containerChooser_.getTypeToCustomType())
    {
        program_.addVariableDeclaration(std::make_unique<Variable>(customType, std::make_unique<ElementaryType>("using"), std::make_unique<SingleValue>(type)));
    }

    auto initialType = std::make_shared<CustomType>("static constexpr int");
    auto initialValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("initial", std::move(initialType), std::move(initialValue), true));
}

template<typename T>
void Compiler::restoreAssignments(const std::unique_ptr<T>& function, std::vector<std::shared_ptr<IAction>> assignments)
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
    for (auto& state : graph->getOuterNodeNames())
    {
        std::string prefix = "state_";
        std::string functionName = prefix + std::to_string(graph->getNodeId(state));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        function->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("mr", "move_representation&"));

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
            for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
            {
                function->addInstruction(std::make_unique<CustomInstruction>(
                    "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), iid)) + "(moves, mr)"));
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

    for (auto [outgoingEdge, iid] : outgoingEdges)
    {
        if (complementaryEdges.count(outgoingEdge))
        {
            const std::string shouldCheckVarName = "should_check_" + outgoingEdge->toName();
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(false, shouldCheckVarName));
            ifInstruction->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), iid)) + "(moves, mr)"));
            function->addInstruction(std::move(ifInstruction));
        }
        else if (const auto complementaryEdge = findComplementaryEdge(outgoingEdge, outgoingEdges))
        {
            complementaryEdges.insert(complementaryEdge);
            const std::string sizeVarName = "size_before_" + outgoingEdge->toName();
            const std::string shouldCheckVarName = "should_check_" + complementaryEdge->toName();
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(sizeVarName, "moves.size()", "const auto"));
            function->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), iid)) + "(moves, mr)"));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                shouldCheckVarName, "(" + sizeVarName + "==moves.size())", "const auto"));
        }
        else
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), iid)) + "(moves, mr)"));
        }
    }
}

void Compiler::generateBoolStateFunctions(
    const std::string& from, const std::string& to, const std::shared_ptr<Graph>& graph, int patternId)
{
    std::string name = patternIdToPrefixName[patternId];
    std::string cacheName = "cache";

    for (auto& state : graph->getOuterNodeNames())
    {
        std::string prefix = "is_legal_" + name + std::to_string(graph_->getNodeId(from)) + "_" +
                             std::to_string(graph_->getNodeId(to)) + "_";
        std::string functionName = prefix + std::to_string(graph_->getNodeId(state));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
        function->addArgument(std::make_unique<VariableDeclarationInstruction>(
            cacheName, containerChooser_.getContainerDeclaration({from, to, patternId}) + "&"));

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        std::unique_ptr<IfInstruction> checkCache =
            std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                false,
                cacheName + "." +
                    containerChooser_.getIsSetDeclaration({from, to, patternId}, graph_->getNodeId(state))));
        checkCache->addInstruction(std::make_unique<ReturnInstruction>("false"));
        function->addInstruction(std::move(checkCache));

        function->addInstruction(std::make_unique<CustomInstruction>(
            cacheName + "." + containerChooser_.getIsSetDeclaration({from, to, patternId}, graph_->getNodeId(state)) +
            ";"));

        const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

        if (outgoingEdges.empty() ||
            (optConditionsReachability_ && isAnyPairOfEdgesComplementary(outgoingEdges) && patternId == 0))
        {
            if (patternId == 2)
            {
                function->addInstruction(
                    std::make_unique<CustomInstruction>("currentState = " + std::to_string(graph_->getNodeId(state))));
            }
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            for (auto& [outgoingEdge, iid] : outgoingEdges)
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        false,
                        prefix + "edge_" + std::to_string(graph->getEdgeId(state, outgoingEdge->toName(), iid)) + "(" +
                            cacheName + ")"));
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
    for (const auto& [stateFrom, stateTo, iid] : graph->getEdgeNames())
    {
        std::string prefix = "edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("mr", "move_representation&"));

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        const auto& actions = graph->getActions(stateFrom, stateTo, iid);
        std::vector<std::shared_ptr<IAction>> assignmentActions;

        bool pushed = false;
        bool playerChanged = false;

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment)
            {
                if (action->getLeftSide() == "player")
                {
                    if (!optConditionsMoveCompression_ || graph->getNumberOfOutgoingEdges(stateFrom) > 1 ||
                        graph->getNodeId(stateFrom) == graph->getNodeId("begin"))
                    {
                        function->addInstruction(std::make_unique<CustomInstruction>(
                            "mr.push_back(" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid)) + ")"));
                        pushed = true;
                    }

                    function->addInstruction(std::make_unique<CustomInstruction>("moves.push_back(mr)"));

                    if (pushed)
                    {
                        function->addInstruction(std::make_unique<CustomInstruction>("mr.pop_back()"));
                    }
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
                        !action->getNegated(), action->getLeftSide(), action->getRightSide()));
                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
                function->addInstruction(std::move(ifInstruction));
            }
            else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
            {
                std::string patterType;
                int patternId = 0;

                if (action->getType() == ActionType::PatternAny)
                {
                    patterType = "any_";
                    patternId = 1;
                }

                std::string fromNode = std::to_string(graph->getNodeId(action->getLeftSide()));
                std::string toNode = std::to_string(graph->getNodeId(action->getRightSide()));
                std::tuple<std::string, std::string, int> typeId = {
                    action->getLeftSide(), action->getRightSide(), patternId};
                std::string cacheName = "cache_" + patterType + fromNode + "_" + toNode;

                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        action->getNegated() ? false : true,
                        "is_legal_" + patterType + fromNode + "_" + toNode + "_" + fromNode + "(" + cacheName + ")"));
                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());

                function->addInstruction(
                    std::make_unique<CustomInstruction>(containerChooser_.getContainerDeclaration(typeId) + cacheName));
                function->addInstruction(std::move(ifInstruction));
            }
        }

        if (playerChanged)
        {
            continue;
        }

        if (!optConditionsMoveCompression_ || graph->getNumberOfOutgoingEdges(stateFrom) > 1 ||
            graph->getNodeId(stateFrom) == graph->getNodeId("begin"))
        {
            function->addInstruction(std::make_unique<CustomInstruction>(
                "mr.push_back(" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid)) + ")"));
            pushed = true;
        }

        function->addInstruction(
            std::make_unique<CustomInstruction>("state_" + std::to_string(graph->getNodeId(stateTo)) + "(moves, mr)"));

        restoreAssignments<Function>(function, assignmentActions);

        if (pushed)
        {
            function->addInstruction(std::make_unique<CustomInstruction>("mr.pop_back()"));
        }

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateBoolEdgeFunctions(
    const std::string& from, const std::string& to, const std::shared_ptr<Graph>& graph, int patternId)
{
    const auto& edges = graph->getEdgeNames();

    std::string name = patternIdToPrefixName[patternId];
    std::tuple<std::string, std::string, int> typeId = {from, to, patternId};
    std::string cacheName = "cache";

    for (const auto& [stateFrom, stateTo, iid] : edges)
    {
        const auto& actions = graph->getActions(stateFrom, stateTo, iid);
        std::vector<std::shared_ptr<IAction>> assignmentActions;

        std::string prefix = "is_legal_" + name + std::to_string(graph_->getNodeId(from)) + "_" +
                             std::to_string(graph_->getNodeId(to)) + "_";
        std::string functionName = prefix + "edge_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
        function->addArgument(std::make_unique<VariableDeclarationInstruction>(
            cacheName, containerChooser_.getContainerDeclaration(typeId) + "&"));

        const auto& innerNodes = graph->getEdge(stateFrom, stateTo, iid)->getInnerNodes();

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
                        !action->getNegated(), action->getLeftSide(), action->getRightSide()));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
                function->addInstruction(std::move(ifInstruction));
            }
            else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
            {
                std::string patterType;
                int innerPatternId = 0;

                if (action->getType() == ActionType::PatternAny)
                {
                    patterType = "any_";
                    innerPatternId = 1;
                }

                std::string fromNode = std::to_string(graph_->getNodeId(action->getLeftSide()));
                std::string toNode = std::to_string(graph_->getNodeId(action->getRightSide()));

                std::string innerCacheName = "cache_" + patterType + fromNode + "_" + toNode;
                std::tuple<std::string, std::string, int> innerTypeId = {
                    action->getLeftSide(), action->getRightSide(), innerPatternId};

                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        action->getNegated() ? false : true,
                        "is_legal_" + patterType + fromNode + "_" + toNode + "_" + fromNode + "(" + innerCacheName +
                            ")"));

                restoreAssignments<IfInstruction>(ifInstruction, assignmentActions);

                ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));

                function->addInstruction(std::make_unique<CustomInstruction>(
                    containerChooser_.getContainerDeclaration(innerTypeId) + innerCacheName));
                function->addInstruction(std::move(ifInstruction));
            }
        }

        function->addInstruction(std::make_unique<AssignmentInstruction>(
            "tmp", prefix + std::to_string(graph_->getNodeId(stateTo)) + "(" + cacheName + ")", "bool"));

        if (patternId != 0 && !assignmentActions.empty())
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(false, "tmp"));

            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
            function->addInstruction(std::move(ifInstruction));
        }

        restoreAssignments<Function>(function, assignmentActions);

        function->addInstruction(std::make_unique<ReturnInstruction>("tmp"));

        program_.addFunction(std::move(function));
    }
}

void Compiler::generateApplyEdgeFunctions(const std::shared_ptr<Graph>& graph)
{
    for (const auto& [stateFrom, stateTo, iid] : graph->getEdgeNames())
    {
        std::string prefix = "apply_edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_)
        {
            function->addInstruction(debugInstruction(prefix + stateFrom + "_" + stateTo));
        }

        bool emptyFunction = true;

        for (const auto& action : graph->getActions(stateFrom, stateTo, iid))
        {
            if (action->getType() == ActionType::Assignment)
            {
                function->addInstruction(
                    std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
                emptyFunction = false;
            }
        }

        if (!emptyFunction)
        {
            if (optConditionsMoveCompression_)
            {
                if (graph->getNumberOfOutgoingEdges(stateTo) == 1)
                {
                    const auto& [edge, iid] = graph->getUnambiguousNotEmptyEdge(stateTo);

                    if (iid == -1)
                    {
                        program_.addFunction(std::move(function));
                        continue;
                    }

                    function->addInstruction(std::make_unique<CustomInstruction>(
                        prefix + std::to_string(graph->getEdgeId(edge->fromName(), edge->toName(), iid)) + "()"));
                }
            }

            program_.addFunction(std::move(function));
        }
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
        std::string id = std::to_string(graph->getNodeId(stateTo));

        if (optConditionsMoveCompression_)
        {
            if (graph->getNumberOfIncomingEdges(stateFrom) != 0 && graph->getNumberOfOutgoingEdges(stateFrom) == 1)
            {
                continue;
            }

            const auto& action = graph->getEdge(stateFrom, stateTo, edgeId)->getActions().back();

            if (!(action->getType() == ActionType::Assignment && action->getLeftSide() == "player"))
            {
                const auto path = graph->getUnambiguousPathFromNode(stateTo, true);

                if (path.size())
                {
                    id = std::to_string(graph->getNodeId(std::get<1>(path.back())));
                }
            }
        }

        sw->addCaseInstruction(
            graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(std::make_unique<ReturnInstruction>(id)));
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
    std::string instructionStr;

    for (const auto& [stateFrom, stateTo, edgeId] : graph->getEdgeNames())
    {
        if (optConditionsMoveCompression_)
        {
            if (graph->getNumberOfIncomingEdges(stateFrom) == 0 || graph->getNumberOfOutgoingEdges(stateFrom) != 1)
            {
                bool emptyFunction = true;

                for (const auto& action : graph->getActions(stateFrom, stateTo, edgeId))
                {
                    if (action->getType() == ActionType::Assignment)
                    {
                        auto block = std::make_unique<BlockInstruction>();
                        block->addInstruction(std::make_unique<CustomInstruction>(
                            "apply_edge_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId)) + "()"));
                        block->addInstruction(std::make_unique<ReturnInstruction>());

                        sw->addCaseInstruction(graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(block));
                        emptyFunction = false;
                        break;
                    }
                }

                if (!emptyFunction || graph->getNumberOfOutgoingEdges(stateTo) != 1)
                {
                    continue;
                }

                const auto& [edge, id] = graph->getUnambiguousNotEmptyEdge(stateTo);

                if (id == -1)
                {
                    continue;
                }

                instructionStr =
                    "apply_edge_" + std::to_string(graph->getEdgeId(edge->fromName(), edge->toName(), id)) + "()";
            }
        }
        else
        {
            bool emptyFunction = true;

            for (const auto& action : graph->getActions(stateFrom, stateTo, edgeId))
            {
                if (action->getType() == ActionType::Assignment)
                {
                    emptyFunction = false;
                    break;
                }
            }

            if (emptyFunction)
            {
                continue;
            }

            instructionStr = "apply_edge_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, edgeId)) + "()";
        }

        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(std::make_unique<CustomInstruction>(instructionStr));
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
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("mr", "move_representation&"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (const auto& [edge, iid] : graph->getEdgeWithActionChangePlayer())
    {
        auto block = std::make_unique<BlockInstruction>();
        block->addInstruction(std::make_unique<CustomInstruction>(
            "state_" + std::to_string(graph->getNodeId(edge->toName())) + "(moves, mr)"));
        block->addInstruction(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getNodeId(edge->toName()), std::move(block));
    }

    auto block = std::make_unique<BlockInstruction>();
    block->addInstruction(
        std::make_unique<CustomInstruction>("state_" + std::to_string(graph->getNodeId("begin")) + "(moves, mr)"));
    block->addInstruction(std::make_unique<ReturnInstruction>());

    sw->addCaseInstruction(graph->getNodeId("begin"), std::move(block));

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
    getPlayerScore->addInstruction(std::make_unique<ReturnInstruction>("goals[player - 1]"));

    auto getCurrentPlayer = std::make_unique<Function>("getCurrentPlayer", "PlayerOrKeeper", true);
    getCurrentPlayer->addInstruction(std::make_unique<ReturnInstruction>("player"));

    auto getCurrentState = std::make_unique<Function>("getCurrentState", "std::string", true);
    getCurrentState->addInstruction(std::make_unique<ReturnInstruction>("std::to_string(currentState)"));

    auto getAllMovesFunction = std::make_unique<Function>("getAllMoves", "void", true);
    getAllMovesFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
    getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(moves.clear();
    move_representation mr;
    runState(currentState, moves, mr))"));

    auto applyMoveFunction = std::make_unique<Function>("applyMove", "void", true);
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("m", "const Move&"));
    applyMoveFunction->addInstruction(std::make_unique<CustomInstruction>(
        R"(const move_representation &v = m.mr;

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

void Compiler::generateApplyAnyMove()
{
    generatePatternFunctions(applyAnyMoveGraphs_, 2);

    auto function = std::make_unique<Function>("applyAnyMove", "bool", true);
    auto sw = std::make_unique<SwitchInstruction>("currentState");

    auto getNodesForApplyAnyMoveFunction =
        [](const std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& v) {
            std::map<std::string, std::set<std::string>> res;

            for (const auto& [fromName, toName, graph] : v)
            {
                assert(res[fromName].find(toName) == res[fromName].end());
                res[fromName].insert(toName);
            }

            return res;
        };

    auto nodesForApplyAnyMove = getNodesForApplyAnyMoveFunction(applyAnyMoveGraphs_);
    for (const auto& [nodeName, nodesToPlayerChangeOrEnd] : nodesForApplyAnyMove)
    {
        auto block = std::make_unique<BlockInstruction>();
        for (const auto& nodeTo : nodesToPlayerChangeOrEnd)
        {
            std::string cacheName = "cache_" + nodeName + "_" + nodeTo;
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    false,
                    "is_legal_any2_" + std::to_string(graph_->getNodeId(nodeName)) + "_" +
                        std::to_string(graph_->getNodeId(nodeTo)) + "_" + std::to_string(graph_->getNodeId(nodeName)) +
                        "(" + cacheName + ")"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
            function->addInstruction(std::make_unique<CustomInstruction>(
                containerChooser_.getContainerDeclaration({nodeName, nodeTo, 2}) + cacheName));
            block->addInstruction(std::move(ifInstruction));
        }
        block->addInstruction(std::make_unique<ReturnInstruction>("false"));
        sw->addCaseInstruction(graph_->getNodeId(nodeName), std::move(block));
    }
    function->addInstruction(std::move(sw));
    function->addInstruction(std::make_unique<ReturnInstruction>("false"));
    program_.addFunction(std::move(function));
}

void Compiler::generatePatternFunctions(
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, int patternId)
{
    for (const auto& [from, to, graph] : patterns)
    {
        generateBoolStateFunctions(from, to, graph, patternId);
    }

    for (const auto& [from, to, graph] : patterns)
    {
        generateBoolEdgeFunctions(from, to, graph, patternId);
    }
}

void Compiler::generatePatternReachabilityFunctions()
{
    generatePatternFunctions(patternReachabilityGraphs_);
}

void Compiler::generatePatternAnyFunctions()
{
    generatePatternFunctions(patternAnyGraphs_, 1);
}

void Compiler::generateFunctions()
{
    generateApplyEdgeFunctions(graph_);
    generateVoidStateFunctions(graph_);

    generatePatternReachabilityFunctions();
    generatePatternAnyFunctions();
    generateApplyAnyMove();

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
    throw std::runtime_error("Illegal kind of type (not implemented): " + t["kind"].get<std::string>());
}

std::shared_ptr<IType> Compiler::generateFunctionType(const nlohmann::json& functionType)
{
    auto sourceType = generateType(functionType["lhs"]);
    auto destinationType = generateType(functionType["rhs"]);
    const std::string sourceTypeName = sourceType->identifier;
    return std::make_shared<FunctionType>(
        std::move(sourceType), std::move(destinationType), valueAssigner_.getTypeRange(sourceTypeName));
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

int Compiler::getNumberOfPlayers()
{
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["identifier"] == "Player")
        {
            return t["type"]["identifiers"].size();
        }
    }
    throw std::runtime_error("Cannot find 'Player' type in AST.");
}
