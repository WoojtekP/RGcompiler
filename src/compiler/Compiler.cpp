#include <functional>
#include <iostream>

#include <compiler/Compiler.hpp>
#include <parser/Parser.hpp>
#include <printer/Printer.hpp>

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
  optConditionsMoveCompression_(options.moveCompression_), temporaryVariableNamePrefix_("old"),
  optNoCycleDetection_(options.noCycleDetection_), mainCacheName_("rgCache"), mainCacheType_("RgCache"),
  containerChooser_(mainCacheType_)
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

    if (!optNoCycleDetection_)
    {
        for (const auto& [from, to, graph] : patterns)
        {
            variablesInPatternGraphs_[std::make_tuple(from, to, patternId)] = std::set<std::string>();
            graph->getVariablesInPatternGraphs(variablesInPatternGraphs_.at({from, to, patternId}));
            const auto& variables = variablesInPatternGraphs_.at({from, to, patternId});
            std::vector<std::pair<std::string, int>> variableAndDomain;
            for (const auto& name : variables)
            {
                variableAndDomain.emplace_back(std::make_pair(name, getDomain(name)));
            }
            containerChooser_.add({from, to, patternId}, variableAndDomain, graph_->getMaximalNodeId());
        }
    }
}

// TODO this function need to be tested
int Compiler::getDomain(const std::string& s)
{
    int check = 0;
    int open = 0;
    int cnt = 0;
    for (int i = 0; i < s.size(); i++)
    {
        char c = s[i];
        if (c == '[')
        {
            if (check == 0)
            {
                check = i;
            }
            if (open == 0)
            {
                cnt++;
            }
            open++;
        }
        else if (c == ']')
        {
            open--;
        }
    }

    std::string k = s;

    if (check > 0)
    {
        k = s.substr(0, check);
    }

    std::string type = parser_.findTypeOfVariable(k)["identifier"];

    while (cnt)
    {
        type = parser_.getDestinationType(parser_.findTypeByIdentifier(type)["type"])["identifier"];
        cnt--;
    }

    if (parser_.findTypeByIdentifier(type)["type"]["kind"] == "Arrow")
    {
        return -1;
    }

    return valueAssigner_.getTypeDomainSize(type);
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
    printer.printAdditionDataForCycleHandling(containerChooser_.getAdditionalData());
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
    for (const auto& [id, customDeclaration] : containerChooser_.getIdTypeToCustomDeclaration())
    {
        program_.addVariableDeclaration(std::make_unique<Variable>(
            customDeclaration,
            std::make_unique<ElementaryType>("using"),
            std::make_unique<SingleValue>(containerChooser_.getContainerDeclaration(id))));
    }

    auto initialType = std::make_shared<CustomType>("static constexpr int");
    auto initialValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("initial", std::move(initialType), std::move(initialValue), true));
}

template<typename T>
void Compiler::restoreAssignments(
    const std::unique_ptr<T>& function, std::vector<std::shared_ptr<IAction>> assignments, int edgeId)
{
    int cnt = 0;
    std::reverse(assignments.begin(), assignments.end());
    for (const auto& action : assignments)
    {
        function->addInstruction(
            std::make_unique<AssignmentInstruction>(action->getLeftSide(), getTemporaryVariableName(cnt, edgeId)));
        cnt++;
    }
}

std::string Compiler::getTemporaryVariableName(int idx, int edgeId)
{
    return temporaryVariableNamePrefix_ + std::to_string(edgeId) + "_" + std::to_string(idx);
}

void Compiler::generateVoidStateFunctions(const std::shared_ptr<Graph>& graph)
{
    for (auto& node : graph->getOuterNodes())
    {
        const std::string state = node->toString();
        std::string prefix = "state_";
        std::string functionArguments = "moves, mr";

        std::string name = std::to_string(graph->getNodeId(state));
        if (debugFlag_ == 2)
        {
            name = state;
        }
        std::string functionName = prefix + name;
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        // if (const auto& optBinding = node->getBinding())
        // {
        //     function->addArgument(std::make_unique<VariableDeclarationInstruction>(
        //         optBinding->getVariableName(), optBinding->getTypeName()));
        // }

        function->addArgument(
            std::make_unique<VariableDeclarationInstruction>("moves", "[[maybe_unused]] std::vector<Move>&"));
        function->addArgument(
            std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]] move_representation&"));

        if (!optNoCycleDetection_)
        {
            functionArguments += "," + mainCacheName_;
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
        }

        if (debugFlag_ == 1)
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
                function->addInstruction(generateVoidEdgeInstruction(graph, state, outgoingEdge->toName(), iid));
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

    std::string functionArguments = "moves, mr";

    if (!optNoCycleDetection_)
    {
        functionArguments += "," + mainCacheName_;
    }

    for (auto [outgoingEdge, iid] : outgoingEdges)
    {
        if (complementaryEdges.count(outgoingEdge))
        {
            const std::string shouldCheckVarName = "should_check_" + outgoingEdge->toName();
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(false, shouldCheckVarName));
            ifInstruction->addInstruction(generateVoidEdgeInstruction(graph, state, outgoingEdge->toName(), iid));
            function->addInstruction(std::move(ifInstruction));
        }
        else if (const auto complementaryEdge = findComplementaryEdge(outgoingEdge, outgoingEdges))
        {
            complementaryEdges.insert(complementaryEdge);
            const std::string sizeVarName = "size_before_" + outgoingEdge->toName();
            const std::string shouldCheckVarName = "should_check_" + complementaryEdge->toName();
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(sizeVarName, "moves.size()", "const auto"));
            function->addInstruction(generateVoidEdgeInstruction(graph, state, outgoingEdge->toName(), iid));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                shouldCheckVarName, "(" + sizeVarName + "==moves.size())", "const auto"));
        }
        else
        {
            function->addInstruction(generateVoidEdgeInstruction(graph, state, outgoingEdge->toName(), iid));
        }
    }
}

void Compiler::generateBoolStateFunctions(
    const std::string& from, const std::string& to, const std::shared_ptr<Graph>& graph, int patternId)
{
    std::string name = patternIdToPrefixName[patternId];
    std::string cacheName = "cache";
    std::string prefix = "is_legal_" + name;
    for (auto& node : graph->getOuterNodes())
    {
        const std::string state = node->toString();
        std::string functionName = prefix + std::to_string(graph_->getNodeId(from)) + "_" +
                                   std::to_string(graph_->getNodeId(to)) + "_" +
                                   std::to_string(graph_->getNodeId(state));

        if (debugFlag_ == 2)
        {
            functionName = prefix + from + "_" + to + "_" + state;
        }

        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (debugFlag_ == 1)
        {
            function->addInstruction(debugInstruction(functionName));
        }

        if (!optNoCycleDetection_)
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                cacheName, containerChooser_.getCustomName({from, to, patternId}) + "&"));
            std::unique_ptr<IfInstruction> checkCache =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    false,
                    cacheName + "." +
                        containerChooser_.getIsSetMethodDeclaration({from, to, patternId}, graph_->getNodeId(state))));
            checkCache->addInstruction(std::make_unique<ReturnInstruction>("false"));
            function->addInstruction(std::move(checkCache));

            function->addInstruction(std::make_unique<CustomInstruction>(
                cacheName + "." +
                containerChooser_.getSetMethodDeclaration({from, to, patternId}, graph_->getNodeId(state))));
        }

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
                function->addInstruction(
                    generateBoolEdgeInstruction(from, to, graph, state, outgoingEdge->toName(), iid, patternId));
            }

            function->addInstruction(std::make_unique<ReturnInstruction>("false"));
        }

        program_.addFunction(std::move(function));
    }
}

std::unique_ptr<BlockInstruction> Compiler::addActionPattern(
    const std::shared_ptr<IAction>& action,
    const std::shared_ptr<Graph>& graph,
    const std::string& stateFrom,
    const std::string& stateTo,
    int iid,
    std::unique_ptr<BlockInstruction> blockInstruction)
{
    std::string patterType;
    int patternId = 0;

    if (action->getType() == ActionType::PatternAny)
    {
        patterType = "any_";
        patternId = 1;
    }

    std::string fromNode = std::to_string(graph_->getNodeId(action->getLeftSide()));
    std::string toNode = std::to_string(graph_->getNodeId(action->getRightSide()));
    std::string prefix = "is_legal_" + patterType;
    std::string functionName = prefix + fromNode + "_" + toNode + "_" + fromNode;

    if (debugFlag_ == 2)
    {
        functionName = prefix + action->getLeftSide() + "_" + action->getRightSide() + "_" + action->getLeftSide();
    }
    std::tuple<std::string, std::string, int> typeId = {action->getLeftSide(), action->getRightSide(), patternId};
    std::string cacheName = "cache_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid)) + "_" + patterType +
                            fromNode + "_" + toNode;
    std::string functionArguments;
    auto tmpBlockInstruction = std::make_unique<BlockInstruction>();

    if (!optNoCycleDetection_)
    {
        functionArguments += mainCacheName_ + "," + cacheName;
        std::string cacheDecl = containerChooser_.getCustomName(typeId);
        if (containerChooser_.isInCache(typeId))
        {
            cacheDecl += "&" + cacheName + "=" + mainCacheName_ + "." + containerChooser_.getFromCache(typeId);
            cacheDecl += ";\n" + cacheName + ".reset(" + containerChooser_.getType(typeId) + ")";
        }
        else
        {
            cacheDecl += " " + cacheName;
        }
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(cacheDecl));
    }

    std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
        std::make_unique<ComparisonInstruction>(action->getNegated(), functionName + "(" + functionArguments + ")"));

    ifInstruction->addInstruction(std::move(blockInstruction));
    tmpBlockInstruction->pushInstructionBack(std::move(ifInstruction));
    return std::move(tmpBlockInstruction);
}

std::unique_ptr<BlockInstruction> Compiler::prepareBaseInstructions(
    const std::shared_ptr<Graph>& graph,
    std::vector<std::shared_ptr<IAction>>& actions,
    std::string stateFrom,
    std::string stateTo,
    int iid)
{
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    bool mrPused = false;
    if (!optConditionsMoveCompression_ || graph->getNumberOfOutgoingEdges(stateFrom) > 1 ||
        graph->getNodeId(stateFrom) == graph->getNodeId("begin"))
    {
        blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(
            "mr.push_back(" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid)) + ")"));
        mrPused = true;
    }

    if (actions.back()->getType() == ActionType::Assignment && actions.back()->getLeftSide() == "player")
    {
        blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("moves.push_back(mr)"));
        actions.pop_back();
    }
    else
    {
        std::string stateFunctionArguments = "moves, mr";

        if (!optNoCycleDetection_)
        {
            stateFunctionArguments += "," + mainCacheName_;
        }

        std::string functionName = std::to_string(graph->getNodeId(stateTo));

        if (debugFlag_ == 2)
        {
            functionName = stateTo;
        }

        blockInstruction->pushInstructionBack(
            std::make_unique<CustomInstruction>("state_" + functionName + "(" + stateFunctionArguments + ")"));
    }

    if (mrPused)
    {
        blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("mr.pop_back()"));
    }

    return blockInstruction;
}

std::unique_ptr<BlockInstruction> Compiler::generateVoidEdgeInstruction(
    const std::shared_ptr<Graph>& graph, const std::string& stateFrom, const std::string& stateTo, int iid)
{
    const auto& baseActions = graph->getActions(stateFrom, stateTo, iid);
    std::vector<std::shared_ptr<IAction>> actions(baseActions.begin(), baseActions.end());
    std::unique_ptr<BlockInstruction> blockInstruction =
        prepareBaseInstructions(graph, actions, stateFrom, stateTo, iid);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);

    for (auto action_iterator = actions.rbegin(); action_iterator != actions.rend(); action_iterator++)
    {
        auto action = *action_iterator;

        if (action->getType() == ActionType::Assignment)
        {
            blockInstruction->pushInstructionFront(
                std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
            blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
                getTemporaryVariableName(temporaryVariableCnt, edgeId), action->getLeftSide(), "const auto"));
            blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
                action->getLeftSide(), getTemporaryVariableName(temporaryVariableCnt, edgeId)));
            temporaryVariableCnt++;
        }
        else if (action->getType() == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    action->getNegated(), action->getLeftSide(), action->getRightSide()));

            ifInstruction->addInstruction(std::move(blockInstruction));
            blockInstruction = std::make_unique<BlockInstruction>();
            blockInstruction->pushInstructionBack(std::move(ifInstruction));
        }
        else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
        {
            blockInstruction = addActionPattern(action, graph, stateFrom, stateTo, iid, std::move(blockInstruction));
        }
    }

    return blockInstruction;
}

std::unique_ptr<BlockInstruction> Compiler::prepareBaseInstructions(
    const std::shared_ptr<Graph>& graph,
    const std::vector<std::shared_ptr<IAction>>& actions,
    const std::string& stateFrom,
    const std::string& stateTo,
    int iid,
    const std::string& cacheName,
    const std::string& prefix,
    int patternId)
{
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    std::string functionArguments;
    if (!optNoCycleDetection_)
    {
        functionArguments = mainCacheName_ + "," + cacheName;
    }
    std::string name = std::to_string(graph_->getNodeId(stateTo));
    if (debugFlag_ == 2)
    {
        name = stateTo;
    }
    std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
        std::make_unique<ComparisonInstruction>(false, prefix + name + "(" + functionArguments + ")"));

    if (patternId == 0)
    {
        std::vector<std::shared_ptr<IAction>> assignmentActions;

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment)
            {
                assignmentActions.push_back(action);
            }
        }
        restoreAssignments<IfInstruction>(ifInstruction, assignmentActions, graph->getEdgeId(stateFrom, stateTo, iid));
    }

    ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
    blockInstruction->pushInstructionBack(std::move(ifInstruction));

    return blockInstruction;
}

std::unique_ptr<BlockInstruction> Compiler::generateBoolEdgeInstruction(
    const std::string& from,
    const std::string& to,
    const std::shared_ptr<Graph>& graph,
    const std::string& stateFrom,
    const std::string& stateTo,
    int iid,
    int patternId)
{
    std::tuple<std::string, std::string, int> typeId = {stateFrom, stateTo, patternId};
    std::string cacheName = "cache";
    std::string name = patternIdToPrefixName[patternId];

    std::string prefix = "is_legal_" + name + std::to_string(graph_->getNodeId(from)) + "_" +
                         std::to_string(graph_->getNodeId(to)) + "_";

    if (debugFlag_ == 2)
    {
        prefix = "is_legal_" + name + from + "_" + to + "_";
    }

    const auto& actions = graph->getActions(stateFrom, stateTo, iid);
    std::unique_ptr<BlockInstruction> blockInstruction =
        prepareBaseInstructions(graph, actions, stateFrom, stateTo, iid, cacheName, prefix, patternId);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);

    for (auto action_iterator = actions.rbegin(); action_iterator != actions.rend(); action_iterator++)
    {
        auto action = *action_iterator;

        if (action->getType() == ActionType::Assignment)
        {
            blockInstruction->pushInstructionFront(
                std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
            blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
                getTemporaryVariableName(temporaryVariableCnt, edgeId), action->getLeftSide(), "const auto"));
            blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
                action->getLeftSide(), getTemporaryVariableName(temporaryVariableCnt, edgeId)));
            temporaryVariableCnt++;
        }
        else if (action->getType() == ActionType::Comparison)
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    action->getNegated(), action->getLeftSide(), action->getRightSide()));

            ifInstruction->addInstruction(std::move(blockInstruction));
            blockInstruction = std::make_unique<BlockInstruction>();
            blockInstruction->pushInstructionBack(std::move(ifInstruction));
        }
        else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
        {
            blockInstruction = addActionPattern(action, graph, stateFrom, stateTo, iid, std::move(blockInstruction));
        }
    }

    return blockInstruction;
}

void Compiler::generateApplyEdgeFunctions(const std::shared_ptr<Graph>& graph)
{
    for (const auto& [edge, iid] : graph->getAllEdges())
    {
        const auto& stateFrom = edge->fromName();
        const auto& stateTo = edge->toName();

        std::string prefix = "apply_edge_";
        std::string functionName = prefix + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));

        if (debugFlag_ == 2)
        {
            functionName = prefix + stateFrom + "_" + stateTo + "_" + std::to_string(iid);
        }

        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "void");

        if (debugFlag_ == 1)
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

        // TODO fix case with bindings in inner nodes
        // if (const auto& leftBinding = edge->getLeftNode()->getBinding())
        // {
        //     function->addArgument(std::make_unique<VariableDeclarationInstruction>(
        //         leftBinding->getVariableName(), leftBinding->getTypeName()));
        // }
        // if (const auto& rightBinding = edge->getRightNode()->getBinding())
        // {
        //     function->addArgument(std::make_unique<VariableDeclarationInstruction>(
        //         rightBinding->getVariableName(), rightBinding->getTypeName()));
        // }

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

                    std::string innerFunctionName =
                        std::to_string(graph->getEdgeId(edge->fromName(), edge->toName(), iid));

                    if (debugFlag_ == 2)
                    {
                        innerFunctionName = edge->fromName() + "_" + edge->toName() + "_" + std::to_string(iid);
                    }

                    function->addInstruction(std::make_unique<CustomInstruction>(prefix + innerFunctionName + "()"));
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

    for (const auto& [stateFrom, stateTo, iid] : graph->getEdgeNames())
    {
        std::string functionName = std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        bool isInstructionStrValid = false;
        if (debugFlag_ == 2)
        {
            functionName = stateFrom + "_" + stateTo + "_" + std::to_string(iid);
        }
        if (optConditionsMoveCompression_)
        {
            if (graph->getNumberOfIncomingEdges(stateFrom) == 0 || graph->getNumberOfOutgoingEdges(stateFrom) != 1)
            {
                bool emptyFunction = true;

                for (const auto& action : graph->getActions(stateFrom, stateTo, iid))
                {
                    if (action->getType() == ActionType::Assignment)
                    {
                        auto block = std::make_unique<BlockInstruction>();
                        block->pushInstructionBack(
                            std::make_unique<CustomInstruction>("apply_edge_" + functionName + "()"));
                        block->pushInstructionBack(std::make_unique<ReturnInstruction>());
                        sw->addCaseInstruction(graph->getEdgeId(stateFrom, stateTo, iid), std::move(block));
                        emptyFunction = false;
                        break;
                    }
                }

                if (!emptyFunction || graph->getNumberOfOutgoingEdges(stateTo) != 1)
                {
                    continue;
                }

                const auto [edge, id] = graph->getUnambiguousNotEmptyEdge(stateTo);

                if (id == -1)
                {
                    continue;
                }

                std::string innerFunctionName = std::to_string(graph->getEdgeId(edge->fromName(), edge->toName(), id));
                if (debugFlag_ == 2)
                {
                    innerFunctionName = edge->fromName() + "_" + edge->toName() + "_" + std::to_string(id);
                }
                isInstructionStrValid = true;
                instructionStr = "apply_edge_" + innerFunctionName + "()";
            }
        }
        else
        {
            bool emptyFunction = true;

            for (const auto& action : graph->getActions(stateFrom, stateTo, iid))
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
            isInstructionStrValid = true;
            instructionStr = "apply_edge_" + functionName + "()";
        }

        if (isInstructionStrValid)
        {
            auto block = std::make_unique<BlockInstruction>();
            block->pushInstructionBack(std::make_unique<CustomInstruction>(instructionStr));
            block->pushInstructionBack(std::make_unique<ReturnInstruction>());

            sw->addCaseInstruction(graph->getEdgeId(stateFrom, stateTo, iid), std::move(block));
        }
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
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
    std::string functionArguments = "moves, mr";
    if (!optNoCycleDetection_)
    {
        functionArguments += "," + mainCacheName_;
    }

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (const auto& [edge, iid] : graph->getEdgeWithActionChangePlayer())
    {
        std::string stateName = std::to_string(graph->getNodeId(edge->toName()));
        if (debugFlag_ == 2)
        {
            stateName = edge->toName();
        }
        auto block = std::make_unique<BlockInstruction>();
        block->pushInstructionBack(
            std::make_unique<CustomInstruction>("state_" + stateName + "(" + functionArguments + ")"));
        block->pushInstructionBack(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getNodeId(edge->toName()), std::move(block));
    }

    std::string stateBeginName = std::to_string(graph->getNodeId("begin"));
    if (debugFlag_ == 2)
    {
        stateBeginName = "begin";
    }
    auto block = std::make_unique<BlockInstruction>();
    block->pushInstructionBack(
        std::make_unique<CustomInstruction>("state_" + stateBeginName + "(" + functionArguments + ")"));
    block->pushInstructionBack(std::make_unique<ReturnInstruction>());

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
    getAllMovesFunction->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, mainCacheType_ + "&"));
    getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
        "moves.clear();\nmove_representation mr;\nrunState(currentState, moves,mr," + mainCacheName_ + ")"));

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
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));

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
            std::string functionArguments;
            if (!optNoCycleDetection_)
            {
                functionArguments = mainCacheName_ + ", " + cacheName;
                std::string cacheDecl = containerChooser_.getCustomName({nodeName, nodeTo, 2});
                if (containerChooser_.isInCache({nodeName, nodeTo, 2}))
                {
                    cacheDecl += "&" + cacheName + "=" + mainCacheName_ + "." +
                                 containerChooser_.getFromCache({nodeName, nodeTo, 2});
                    cacheDecl += ";\n" + cacheName + ".reset(" + containerChooser_.getType({nodeName, nodeTo, 2}) + ")";
                }
                else
                {
                    cacheDecl += " " + cacheName;
                }
                function->addInstruction(std::make_unique<CustomInstruction>(cacheDecl));
            }
            std::string functionName = std::to_string(graph_->getNodeId(nodeName)) + "_" +
                                       std::to_string(graph_->getNodeId(nodeTo)) + "_" +
                                       std::to_string(graph_->getNodeId(nodeName));
            if (debugFlag_ == 2)
            {
                functionName = nodeName + "_" + nodeTo + "_" + nodeName;
            }
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    false, "is_legal_any2_" + functionName + "(" + functionArguments + ")"));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
            block->pushInstructionBack(std::move(ifInstruction));
        }
        block->pushInstructionBack(std::make_unique<ReturnInstruction>("false"));
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
        if (entry["kind"] == "ValueEntry")
        {
            if (entry["identifier"].is_null())
            {
                defaultValue = generateValue(entry["value"]);
            }
            else
            {
                idToValueMap.emplace(entry["identifier"].get<std::string>(), generateValue(entry["value"]));
            }
        }
        else
        {
            throw std::runtime_error("Unknown type of map entry: " + entry["kind"].get<std::string>());
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
