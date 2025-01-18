#include <functional>
#include <ranges>

#include <compiler/Compiler.hpp>
#include <compiler/stateCache/IStateCache.hpp>
#include <compiler/stateCache/StateCacheFactory.hpp>
#include <parser/Parser.hpp>
#include <printer/Printer.hpp>
#include <program/LoopFactory.hpp>

namespace
{
const std::string UNUSED_TAG_VALUE = "-1";

std::optional<std::string> getTagVar(const std::string& tag)
{
    assert(tag.size());
    std::string tagTmp = tag.substr(1, tag.size());
    auto pos = tagTmp.find(":");

    if (pos != std::string::npos)
    {
        return tagTmp.substr(0, pos - 1);
    }
    return {};
}

bool isAnyPairOfEdgesComplementary(const EdgesWithIID& edges)
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

std::shared_ptr<Edge> findComplementaryEdge(const std::shared_ptr<Edge>& edge, const EdgesWithIID& edges)
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

std::string getValueInRangeExpressionString(const std::string variable, const int lower, const int upper)
{
    const auto lowerLimitValueStr = std::to_string(lower);
    const auto upperLimitValueStr = std::to_string(upper);
    return variable + " >= " + lowerLimitValueStr + " && " + variable + " <= " + upperLimitValueStr;
}

std::string formatValueForPrinting(
    const std::string& varIdentifier,
    const std::shared_ptr<IType>& valueType,
    IValue* value,
    const ValueAssigner& valueAssigner,
    const Parser& parser)
{
    if (const FunctionType* functionType = dynamic_cast<FunctionType*>(valueType.get()))
    {
        MapValue* mapValue = dynamic_cast<MapValue*>(value);
        std::string streamInstr = "\"{\"";
        const auto sourceTypeId = functionType->source->identifier;
        const auto minValue = valueAssigner.getTypeMinMaxValues(sourceTypeId).first;
        const auto shiftStr = (minValue > 0) ? "-" + std::to_string(minValue) : "";
        const std::string sep = " << \", \"";
        for (const auto& symbol : parser.getDomain(sourceTypeId))
        {
            const auto anyDestinationValue =
                mapValue->defaultValue ? mapValue->defaultValue.get() : mapValue->idToValueMap.begin()->second.get();
            const auto nestedVariable = varIdentifier + "[" + symbol + shiftStr + "]";
            const auto valueStr = formatValueForPrinting(
                nestedVariable, functionType->destination, anyDestinationValue, valueAssigner, parser);
            streamInstr += " << \"" + symbol + ": \" << " + valueStr + sep;
        }
        streamInstr.resize(streamInstr.size() - sep.size());
        streamInstr += "<< \"}\"";
        return streamInstr;
    }
    else
    {
        return varIdentifier;
    }
}

void addReturnInstruction(const std::unique_ptr<IfInstruction>& ifInstruction, const bool isBoolFunction)
{
    if (isBoolFunction)
    {
        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
    }
    else
    {
        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>());
    }
}
}  // namespace

Compiler::Compiler(const Parser& parser, const Options& options)
: parser_(parser)
, printOriginalNames_(options.printOriginalNames_)
, preserveOriginalNames_(options.preserveOriginalNames_)
, pragmaDisjointEnabled_(options.pragmaDisjointEnabled_)
, verification_(options.verification_)
, optConditionsSimplePathCompression_(options.simplePathCompression_)
, optGccInline_(options.gccInline_)
, allUnique_(options.allUnique_)
, maxMoveLen_(options.maxMoveLen_ == -1 ? std::nullopt : std::optional(options.maxMoveLen_))
, temporaryVariableNamePrefix_("old")
, optNoCycleDetection_(options.noCycleDetection_)
, mainCacheName_("rgCache")
, mainCacheType_("RgCache")
, containerChooser_(mainCacheType_)
, graphOperatorManager_(std::make_shared<GraphOperatorManager>())

{
    valueAssigner_.assignValuesForSymbols(parser_.getTypeDeclarations());
    initializeGraph();
    valueAssigner_.assignValuesForTags(graph_->getAllEdges());
    initializePragmas();
    generateStateCaches();
}

void Compiler::compile()
{
    generateTypes();
    generateConstants();
    generateVariables(graph_);
    generateFunctions();
}

void Compiler::initializePragmaVerticesSet(const std::string& pragmaName, std::set<std::string>& data)
{
    for (const auto& pragma : parser_.getPragmas(pragmaName))
    {
        for (const auto& edge : pragma["edgeNames"])
        {
            const auto parts = edge["parts"];
            if (parts.size() == 1)
            {
                data.insert(parts[0]["identifier"].get<std::string>());
            }
            else if (parts.size() == 2)
            {
                const auto nodeName = parts[0]["identifier"].get<std::string>();
                const auto generatorVariable = parts[1]["identifier"].get<std::string>();
                const auto generatorType = parts[1]["type"]["identifier"].get<std::string>();
                data.insert(nodeName + "__bind__" + generatorVariable);
            }
            else
            {
                throw std::runtime_error(
                    "Unhandled number of parts in @unique pragma: " + std::to_string(parts.size()));
            }
        }
    }
}

void Compiler::initializePragmaDisjoint()
{
    graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph_)->init(parser_);
}

void Compiler::initializePragmaUnique()
{
    initializePragmaVerticesSet("Unique", pragmaUniqueData_);

    for (const auto& [from, to, graph] : patternReachabilityGraphs_)
    {
        if (graphOperatorManager_->getOperator<PragmaUniqueOperator>(graph)->areAllNodesWithPragmaUnique(
                pragmaUniqueData_) ||
            allUnique_)
        {
            areAllNodesInPatternGraphUnique_.insert({from, to});
        }
    }

    for (const auto& [from, to, graph] : applyAnyMoveGraphs_)
    {
        if (graphOperatorManager_->getOperator<PragmaUniqueOperator>(graph)->areAllNodesWithPragmaUnique(
                pragmaUniqueData_) ||
            allUnique_)
        {
            areAllNodesInApplyAnyGraphUnique_.insert({from, to});
        }
    }
}

void Compiler::initializePragmaRepeat()
{
    if (allUnique_)
    {
        return;
    }

    const auto isVariableOfFunctionType = [this](const auto& variableName) {
        const auto& variableType = parser_.findTypeOfVariable(variableName);
        if (variableType["kind"] == "TypeReference")
        {
            const auto& typeDefinition = parser_.findTypeByIdentifier(variableType["identifier"]);
            return typeDefinition["type"]["kind"] == "Arrow";
        }
        return variableType["kind"] == "Arrow";
    };
    for (const auto& pragma : parser_.getPragmas("Repeat"))
    {
        if (std::any_of(pragma["identifiers"].begin(), pragma["identifiers"].end(), isVariableOfFunctionType))
        {
            continue;
        }
        for (const auto& edge : pragma["edgeNames"])
        {
            const auto& nodeName = edge["parts"][0]["identifier"];
            pragmaRepeatData_[nodeName] = {};
            for (const auto& variableName : pragma["identifiers"])
            {
                pragmaRepeatData_[nodeName].push_back(variableName);
            }
        }
    }
}

void Compiler::initializePragmaSimpleApply()
{
    graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)->init(parser_, valueAssigner_);
}

void Compiler::initializePragmas()
{
    graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->init(parser_);
    initializePragmaDisjoint();
    initializePragmaUnique();
    initializePragmaRepeat();
    initializePragmaSimpleApply();
}

void Compiler::generateStateCaches()
{
    StateCacheFactory factory(parser_, valueAssigner_);
    for (const auto& [nodeName, variables] : pragmaRepeatData_)
    {
        stateToCache_.emplace(nodeName, std::move(factory.createStateCache(nodeName, variables)));
    }
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

    graph_->initialize(valueAssigner_);

    patternReachabilityGraphs_ =
        graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forPatterns(ActionType::Reachability);
    patternAnyGraphs_ =
        graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forPatterns(ActionType::PatternAny);
    applyAnyMoveGraphs_ = graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forApplyAnyMove();

    if (optConditionsSimplePathCompression_)
    {
        unoptimizedGraph_ =
            graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths(
                valueAssigner_);
        swap(unoptimizedGraph_, graph_);
    }
    else
    {
        unoptimizedGraph_ = graph_;
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
        graph->initialize(valueAssigner_);
    }

    if (optConditionsSimplePathCompression_)
    {
        for (size_t i = 0; i < patterns.size(); i++)
        {
            auto& graph = std::get<2>(patterns[i]);

            patterns[i] = std::make_tuple(
                std::get<0>(patterns[i]),
                std::get<1>(patterns[i]),
                graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph)->getGraphWithOptimizedPaths(
                    valueAssigner_));
        }
    }
}

std::pair<std::string, int> Compiler::getMoveRepresentation()
{
    int containerSize = graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->containerSize();
    if (graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition())
    {
        return {"std::array", containerSize};
    }
    if (containerSize != -1)
    {
        return {"boost::container::static_vector", containerSize};
    }
    if (maxMoveLen_)
    {
        return {"boost::container::static_vector", *maxMoveLen_};
    }
    return {"std::vector", -1};
}

void Compiler::generateSourceCode(
    const std::string& outputFileName, std::ofstream& headerFile, std::ofstream& sourceFile)
{
    Printer printer(parser_, valueAssigner_, outputFileName, headerFile, sourceFile);
    printer.initializeHeaderFile(printOriginalNames_);
    printer.initializeSourceFile();
    printer.printTypeDeclarations(program_.getTypes());
    printer.printSymbolValues();
    printer.printConstants(program_.getConstants());
    printer.printMoveRepresentationDeclaration(getMoveRepresentation());
    printer.printNonGameStateFunctions(program_.getNonGameStateFunctions());
    printer.initializeMainClass();
    printer.printVariables(program_.getVariables(), gameStateHasher_);
    printer.printFunctions(program_.getFunctions());
    printer.endMainClass();
    printer.printMainCache(containerChooser_.getAdditionalData(gameStateAndMoveAndNodeIdHasherBody_, stateToCache_));
    printer.endHeaderFile();
    printer.endSourceFile(gameStateAndMoveAndNodeIdHasherBody_);
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
    std::unique_ptr<Function> comparisionFunction = std::make_unique<Function>("operator==", "bool", "", true, true);

    comparisionFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("gameState", "const GameState&"));

    std::string comparisionFunctionBody;

    for (const auto& variable : parser_.getVariables())
    {
        auto valueType = generateType(variable["type"]);
        auto value = generateValue(variable["defaultValue"]);
        const std::string identifier = variable["identifier"].get<std::string>();
        program_.addVariableDeclaration(
            std::make_unique<Variable>(identifier, std::move(valueType), std::move(value), true));
        comparisionFunctionBody += identifier + " == " + "gameState." + identifier + "&&";
        gameStateAndMoveAndNodeIdHasherBody_ += "hash(std::get<0>(gameState)." + identifier + ") ^";
    }

    if (!comparisionFunctionBody.empty())
    {
        comparisionFunctionBody.pop_back();
        comparisionFunctionBody.pop_back();
        gameStateAndMoveAndNodeIdHasherBody_.pop_back();
    }
    comparisionFunction->addInstruction(std::make_unique<CustomInstruction>("return " + comparisionFunctionBody));

    gameStateAndMoveAndNodeIdHasherBody_ =
        "return hash(std::get<1>(gameState)) ^ " + gameStateAndMoveAndNodeIdHasherBody_;

    gameStateHasher_ = "struct gameStateHasher{size_t operator()(const std::tuple<GameState, int>& gameState) const{";
    gameStateHasher_ += gameStateAndMoveAndNodeIdHasherBody_ + ";}};";

    gameStateAndMoveAndNodeIdHasherBody_ += "^ std::get<2>(gameState);";

    std::string gameStateAndMoveAndNodeIdHasher = "struct {";
    gameStateAndMoveAndNodeIdHasher +=
        "size_t operator()(const std::tuple<GameState,move_representation,int>& gameState) const{";
    gameStateAndMoveAndNodeIdHasher += gameStateAndMoveAndNodeIdHasherBody_;
    gameStateAndMoveAndNodeIdHasher += "}};";
    program_.addVariableDeclaration(std::make_unique<Variable>(
        "gameStateAndMoveAndNodeIdHasher",
        std::make_unique<ElementaryType>("using"),
        std::make_unique<SingleValue>(gameStateAndMoveAndNodeIdHasher)));

    program_.addFunction(std::move(comparisionFunction));

    std::string initialState = std::to_string(graph->getNodeId("begin"));

    auto currentStateType = std::make_shared<CustomType>("int");
    auto currentStateValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));

    auto currentMrIdType = std::make_shared<CustomType>("int");
    auto currentMrIdValue = std::make_unique<SingleValue>("0");
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentMrId", std::move(currentMrIdType), std::move(currentMrIdValue)));

    if (verification_)
    {
        program_.addVariableDeclaration(std::make_unique<Variable>(
            "verificationCache",
            std::move(std::make_shared<CustomType>("std::unordered_map<move_representation, int, vector_hash>"))));
    }

    for (const auto& [id, customDeclaration] : containerChooser_.getIdTypeToCustomDeclaration())
    {
        program_.addVariableDeclaration(std::make_unique<Variable>(
            customDeclaration,
            std::make_unique<ElementaryType>("using"),
            std::make_unique<SingleValue>("std::unordered_set<std::pair<GameState,int>, gameStateHasher>")));

        //  std::make_unique<SingleValue>(containerChooser_.getContainerDeclaration(id))));
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
        std::string lvalue = action->getLeftSide();

        function->addInstruction(
            std::make_unique<AssignmentInstruction>(lvalue, getTemporaryVariableName(cnt, edgeId)));
        cnt++;
    }
}

std::string Compiler::getTemporaryVariableName(int idx, int edgeId)
{
    return temporaryVariableNamePrefix_ + std::to_string(edgeId) + "_" + std::to_string(idx);
}

bool Compiler::nodeInThisEdge(const std::shared_ptr<Edge>& edge, const std::string& nodeName) const
{
    for (const auto& node : edge->getInnerNodes())
    {
        if (nodeName == node->getName())
        {
            return true;
        }
    }

    return nodeName == edge->getRightNode()->getName();
}

void Compiler::generateVoidStateFunctions(const std::shared_ptr<Graph>& graph, bool applyMode)
{
    for (auto& node : graph->getOuterNodes())
    {
        const std::string state = node->toString();
        std::string prefix = "state_";
        bool isSimpleApply = false;
        if (applyMode)
        {
            prefix = "apply_state_";
            isSimpleApply = graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                                ->isSimpleApply(node->getName());
        }

        std::string name = std::to_string(graph->getNodeId(state));
        if (preserveOriginalNames_)
        {
            name = state;
        }
        std::string functionType = "void";
        std::string functionName = prefix + name;
        if (applyMode)
        {
            functionType = "bool";
        }

        std::string attribiutes;
        if (optGccInline_ && pragmaUniqueData_.count(state))
        {
            attribiutes += "__attribute__((always_inline))inline";
        }
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, functionType, attribiutes);

        // if (const auto& optBinding = node->getBinding())
        // {
        //     function->addArgument(std::make_unique<VariableDeclarationInstruction>(
        //         optBinding->getVariableName(), optBinding->getTypeName()));
        // }
        if (applyMode)
        {
            function->addArgument(
                std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]]const move_representation&"));
        }
        else
        {
            function->addArgument(
                std::make_unique<VariableDeclarationInstruction>("moves", "[[maybe_unused]] std::vector<Move>&"));
            function->addArgument(
                std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]] move_representation&"));
        }

        if (!optNoCycleDetection_)
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
        }

        if (printOriginalNames_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        if (pragmaRepeatData_.count(state))
        {
            const auto stateCache = getStateCacheSafe(state);
            const std::string cacheVarName = "cache";
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                cacheVarName, mainCacheName_ + "." + stateCache->getCacheName() + "[mr]", "auto&"));

            auto testCacheInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>(cacheVarName + stateCache->getTestInstruction()));
            addReturnInstruction(testCacheInstruction, applyMode);
            function->addInstruction(std::move(testCacheInstruction));

            const auto insertInstruction = cacheVarName + stateCache->getInsertInstruction();
            function->addInstruction(std::make_unique<CustomInstruction>(insertInstruction));
        }
        else if (!applyMode && !(pragmaUniqueData_.count(node->getName()) || allUnique_))
        {
            auto nodeId = std::to_string(graph_->getNodeId(state));
            if (const auto binding = node->getBinding())
            {
                nodeId += " + " + binding->getVariableName();
            }
            const std::string cacheData = "std::make_tuple(*this, mr, " + nodeId + ")";
            const std::string cacheName = mainCacheName_ + ".state_cache";

            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    cacheName + ".insert(" + cacheData + ").second", ComparisonType::Neg));
            addReturnInstruction(ifInstruction, applyMode);
            function->addInstruction(std::move(ifInstruction));
        }

        bool skipForExhaustiveSimpleApply = false;
        if (applyMode && graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                             ->isMainSimpleApply(node->getName()))
        {
            function->addInstruction(generateVoidEdgeInstruction(
                node,
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->getActionListToTags(node),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->getActionListToPlayerChange(node),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->isExhaustive(node->getName()),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->hasAnyEmptyTagSequence(node->getName())));

            skipForExhaustiveSimpleApply =
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->isExhaustive(node->getName());
        }

        if (!skipForExhaustiveSimpleApply)
        {
            if (pragmaDisjointEnabled_ &&
                graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph)->isDisjoint(state) && !isSimpleApply)
            {
                auto vectorOfNodeNames =
                    graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph)->getNodeNames(state);
                bool disjointExhaustive =
                    graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph)->isExhaustive(state);
                int cnt = 0;
                std::set<std::string> visited;

                visited.insert(vectorOfNodeNames.begin(), vectorOfNodeNames.end());
                for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
                {
                    if (!visited.count(outgoingEdge->getRightNode()->getName()))
                    {
                        function->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
                    }
                }
                visited.clear();

                for (const auto& nodeName : vectorOfNodeNames)
                {
                    if (!visited.insert(nodeName).second)
                    {
                        continue;
                    }
                    for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
                    {
                        if (nodeInThisEdge(outgoingEdge, nodeName))
                        {
                            function->addInstruction(generateVoidEdgeInstruction(
                                graph,
                                outgoingEdge,
                                iid,
                                applyMode,
                                true,
                                disjointExhaustive && ++cnt == vectorOfNodeNames.size()));
                        }
                    }
                }
            }
            else
            {
                for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
                {
                    function->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
                }
            }
        }

        if (applyMode)
        {
            function->addInstruction(std::move(std::make_unique<ReturnInstruction>("false")));
        }

        if (const auto binding = node->getBinding())
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                binding->getVariableName(), "[[maybe_unused]]" + binding->getTypeName()));
        }
        program_.addFunction(std::move(function));
    }
}

std::unique_ptr<BlockInstruction> Compiler::getAssignments(
    const std::vector<std::unique_ptr<IAction>>& actions,
    const std::vector<std::string>& tags,
    std::vector<int>& minValues) const
{
    int curentPos = 1;
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    for (auto tag : tags)
    {
        auto tagVar = getTagVar(tag);
        if (tagVar)
        {
            tag = *tagVar;
            blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
                tag,
                "mr[currentMrId - " + std::to_string(minValues.size() - curentPos + 1) + "]" + " - " +
                    std::to_string(minValues[curentPos - 1]),
                "[[maybe_unused]] auto"));
        }
        curentPos++;
    }

    for (const auto& action : actions)
    {
        blockInstruction->pushInstructionBack(
            std::make_unique<AssignmentInstruction>(action->getLeftSide(), action->getRightSide()));
    }

    return std::move(blockInstruction);
}

std::unique_ptr<BlockInstruction> Compiler::makeSwitchForTags(
    const std::shared_ptr<SimpleApplySwitchTreeNode>& listOfActionsToTags,
    std::vector<std::string>& tags,
    int depth,
    const bool isExhaustive,
    const bool hasAnyEmptyTagSequence,
    std::vector<int>& minValues)
{
    if (listOfActionsToTags->children_.empty())
    {
        std::unique_ptr<BlockInstruction> blockInstructionTmp =
            getAssignments(listOfActionsToTags->listOfActions_, tags, minValues);

        blockInstructionTmp->pushInstructionBack(prepareBaseInstructions(
            unoptimizedGraph_, listOfActionsToTags->listOfActions_, listOfActionsToTags->endNode_, true, true));

        return std::move(blockInstructionTmp);
    }

    auto sw = std::make_unique<SwitchInstruction>("mr[currentMrId++]");
    int cnt = 0;
    for (auto pairFullTagAndChild : listOfActionsToTags->children_)
    {
        const auto [minValue, maxValue] = valueAssigner_.getRangeValueForTag(pairFullTagAndChild.first);
        minValues.push_back(minValue);
        tags.push_back(pairFullTagAndChild.first);
        auto innerInstructions = std::move(makeSwitchForTags(
            pairFullTagAndChild.second, tags, depth + 1, isExhaustive, hasAnyEmptyTagSequence, minValues));
        minValues.pop_back();
        tags.pop_back();
        std::unique_ptr<BlockInstruction> breakInstruction = std::make_unique<BlockInstruction>();
        if (!pairFullTagAndChild.second->children_.empty())
        {
            if (isExhaustive && !hasAnyEmptyTagSequence)
            {
                breakInstruction->pushInstructionBack(std::move(innerInstructions));
            }
            else
            {
                std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                    std::make_unique<ComparisonInstruction>("static_cast<int>(mr.size()) > currentMrId"));
                ifInstruction->addInstruction(std::move(innerInstructions));
                breakInstruction->pushInstructionBack(std::move(ifInstruction));
            }
            breakInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("break"));
        }
        else
        {
            breakInstruction->pushInstructionBack(std::move(innerInstructions));
        }
        if (++cnt == listOfActionsToTags->children_.size() && isExhaustive)
        {
            sw->addDefaultInstruction(std::move(breakInstruction));
        }
        else
        {
            sw->addCaseInstruction(minValue, maxValue, std::move(breakInstruction));
        }
    }

    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();
    blockInstruction->pushInstructionBack(std::move(sw));

    return blockInstruction;
}

std::unique_ptr<BlockInstruction> Compiler::generateVoidEdgeInstruction(
    const std::shared_ptr<Node>& node,
    const std::shared_ptr<SimpleApplySwitchTreeNode>& listOfActionsToTags,
    std::pair<std::vector<std::unique_ptr<IAction>>, std::unique_ptr<Node>>& listOfActionsToPlayerChangeAndEndNode,
    const bool isExhaustive,
    const bool hasAnyEmptyTagSequence)
{
    static int nameCnt = 0;
    std::string functionName = "switch_" + std::to_string(nameCnt++);
    std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]]const move_representation&"));
    const auto& binding = node->getBinding();

    if (!optNoCycleDetection_)
    {
        function->addArgument(std::make_unique<VariableDeclarationInstruction>(
            mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
    }

    if (binding)
    {
        function->addArgument(std::make_unique<VariableDeclarationInstruction>(
            binding->getVariableName(), "[[maybe_unused]]" + binding->getTypeName()));
    }
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    if (!listOfActionsToTags->empty())
    {
        std::vector<int> minValues;
        std::unique_ptr<IInstruction> blockAction;
        std::vector<std::string> tags;
        auto switchBody =
            makeSwitchForTags(listOfActionsToTags, tags, 1, isExhaustive, hasAnyEmptyTagSequence, minValues);
        if (isExhaustive && !hasAnyEmptyTagSequence)
        {
            blockAction = std::move(switchBody);
        }
        else
        {
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size())", "currentMrId", ComparisonType::Gr));
            ifInstruction->addInstruction(std::move(switchBody));
            blockAction = std::move(ifInstruction);
        }

        blockInstruction->pushInstructionFront(std::move(blockAction));
    }

    if (!listOfActionsToPlayerChangeAndEndNode.first.empty())
    {
        std::vector<int> minValuesEmpty;
        std::unique_ptr<BlockInstruction> blockInstructionTmp =
            getAssignments(listOfActionsToPlayerChangeAndEndNode.first, {}, minValuesEmpty);

        blockInstructionTmp->pushInstructionBack(prepareBaseInstructions(
            unoptimizedGraph_,
            listOfActionsToPlayerChangeAndEndNode.first,
            listOfActionsToPlayerChangeAndEndNode.second,
            true));

        blockInstruction->pushInstructionBack(std::move(blockInstructionTmp));

        std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
            std::make_unique<ComparisonInstruction>("static_cast<int>(mr.size())", "currentMrId", ComparisonType::Neq));
        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
        blockInstruction->pushInstructionBack(std::move(ifInstruction));
    }

    if (!isExhaustive || hasAnyEmptyTagSequence)
    {
        blockInstruction->pushInstructionFront(
            std::make_unique<AssignmentInstruction>("const int tmpCurrentMrId", "currentMrId"));
        blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>("currentMrId", "tmpCurrentMrId"));
    }

    function->addInstruction(std::move(blockInstruction));
    function->addInstruction(std::make_unique<ReturnInstruction>("false"));

    program_.addFunction(std::move(function));

    blockInstruction = std::make_unique<BlockInstruction>();
    std::string functionCall =
        functionName + "(mr, " + mainCacheName_ + (binding ? ", " + binding->getVariableName() : "") + ")";
    if (isExhaustive)
    {
        blockInstruction->pushInstructionBack((std::make_unique<ReturnInstruction>(functionCall)));
    }
    else
    {
        std::unique_ptr<IfInstruction> ifInstruction =
            std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(functionCall));
        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));
        blockInstruction->pushInstructionBack(std::move(ifInstruction));
    }
    return std::move(blockInstruction);
}

void Compiler::generateVoidStateOptimizedFunction(
    const std::string& state,
    const std::unique_ptr<Function>& function,
    const std::shared_ptr<Graph>& graph,
    bool applyMode)
{
    std::set<std::shared_ptr<Edge>> complementaryEdges;
    const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

    for (auto [outgoingEdge, iid] : outgoingEdges)
    {
        if (complementaryEdges.count(outgoingEdge))
        {
            const std::string shouldCheckVarName = "should_check_" + outgoingEdge->toName();
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(shouldCheckVarName));
            ifInstruction->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
            function->addInstruction(std::move(ifInstruction));
        }
        else if (const auto complementaryEdge = findComplementaryEdge(outgoingEdge, outgoingEdges))
        {
            complementaryEdges.insert(complementaryEdge);
            const std::string sizeVarName = "size_before_" + outgoingEdge->toName();
            const std::string shouldCheckVarName = "should_check_" + complementaryEdge->toName();
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(sizeVarName, "moves.size()", "const auto"));
            function->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                shouldCheckVarName, "(" + sizeVarName + "==moves.size())", "const auto"));
        }
        else
        {
            function->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
        }
    }
}

void Compiler::generateBoolStateFunctions(
    const std::string& from,
    const std::string& to,
    const std::shared_ptr<Graph>& graph,
    int patternId,
    bool skipStateCache)
{
    std::string name = patternIdToPrefixName[patternId];
    const std::string cacheName = mainCacheName_ + ".pattern_cache[" + mainCacheName_ + ".depth]";
    std::string prefix = "is_legal_" + name;
    for (auto& node : graph->getOuterNodes())
    {
        const std::string state = node->toString();
        std::string functionName = prefix + std::to_string(graph_->getNodeId(from)) + "_" +
                                   std::to_string(graph_->getNodeId(to)) + "_" +
                                   std::to_string(graph_->getNodeId(state));

        if (preserveOriginalNames_)
        {
            functionName = prefix + from + "_" + to + "_" + state;
        }

        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");

        if (printOriginalNames_)
        {
            function->addInstruction(debugInstruction(functionName));
        }

        if (!optNoCycleDetection_)
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));

            if (!(pragmaUniqueData_.count(node->getAlternativeName()) || allUnique_) && !skipStateCache)
            {
                auto nodeId = std::to_string(graph_->getNodeId(state));
                if (const auto binding = node->getBinding())
                {
                    nodeId += " + " + binding->getVariableName();
                }
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        cacheName + ".insert(std::make_tuple(*this, " + nodeId + ")).second", ComparisonType::Neg));
                ifInstruction->addInstruction(std::move(std::make_unique<ReturnInstruction>("false")));

                function->addInstruction(std::move(ifInstruction));
                // function->addInstruction(std::move(std::make_unique<CustomInstruction>(cacheName + ".insert(mr)")));
            }
            // std::unique_ptr<IfInstruction> checkCache =
            //     std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
            //         cacheName + ".count(std::make_pair(*this," + std::to_string(graph_->getNodeId(state)) + "))"));
            // //containerChooser_.getIsSetMethodDeclaration({from, to, patternId}, graph_->getNodeId(state))));
            // checkCache->addInstruction(std::make_unique<ReturnInstruction>("false"));
            // function->addInstruction(std::move(checkCache));

            // function->addInstruction(std::make_unique<CustomInstruction>(
            //     cacheName + ".insert(std::make_pair(*this," + std::to_string(graph_->getNodeId(state)) + "))"));
            //containerChooser_.getSetMethodDeclaration({from, to, patternId}, graph_->getNodeId(state))));
        }

        const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

        if (state == to)
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            int edgeIdx = 0;
            for (auto& [outgoingEdge, iid] : outgoingEdges)
            {
                function->addInstruction(
                    generateBoolEdgeInstruction(from, to, graph, outgoingEdge, iid, edgeIdx++, name));
            }

            function->addInstruction(std::make_unique<ReturnInstruction>("false"));
        }

        if (const auto binding = node->getBinding())
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                binding->getVariableName(), "[[maybe_unused]]" + binding->getTypeName()));
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
    std::unique_ptr<BlockInstruction> blockInstruction,
    std::unique_ptr<CustomInstruction> returnInstruction)
{
    std::string patterType;
    int patternId = 0;

    std::string fromNode = std::to_string(graph_->getNodeId(action->getLeftSide()));
    std::string toNode = std::to_string(graph_->getNodeId(action->getRightSide()));
    std::string prefix = "is_legal_" + patterType;
    std::string functionName = prefix + fromNode + "_" + toNode + "_" + fromNode;

    if (preserveOriginalNames_)
    {
        functionName = prefix + action->getLeftSide() + "_" + action->getRightSide() + "_" + action->getLeftSide();
    }
    std::tuple<std::string, std::string, int> typeId = {action->getLeftSide(), action->getRightSide(), patternId};
    std::string cacheName = "cache_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid)) + "_" + patterType +
                            fromNode + "_" + toNode;
    std::string functionArguments;
    auto tmpBlockInstruction = std::make_unique<BlockInstruction>();

    bool skipStateCache = areAllNodesInPatternGraphUnique_.count({action->getLeftSide(), action->getRightSide()});
    functionArguments += mainCacheName_;
    //     std::string cacheDecl = containerChooser_.getCustomName(typeId);
    //     if (containerChooser_.isInCache(typeId))
    //     {
    //         cacheDecl += "&" + cacheName + "=" + mainCacheName_ + "." + containerChooser_.getFromCache(typeId);
    //         // cacheDecl += ";\n" + cacheName + ".reset(" + containerChooser_.getType(typeId) + ")";
    //     }
    //     else
    //     {
    //         cacheDecl += " " + cacheName;
    //     }
    // tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(cacheDecl));

    const auto functionCall = functionName + "(" + functionArguments + ")";
    std::string comparisonExpression;
    if (!skipStateCache)
    {
        const auto functionResultVar = "result_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".incDepth()"));
        tmpBlockInstruction->pushInstructionBack(
            std::make_unique<AssignmentInstruction>(functionResultVar, functionCall, "const auto"));
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".decDepth()"));
        comparisonExpression = functionResultVar;
    }
    else
    {
        comparisonExpression = functionCall;
    }

    const auto cmpType = action->getNegated() ? ComparisonType::Neg : ComparisonType::None;
    std::unique_ptr<IfInstruction> ifInstruction =
        std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(comparisonExpression, cmpType));

    ifInstruction->addInstruction(std::move(blockInstruction));
    if (returnInstruction)
    {
        ifInstruction->addInstruction(std::move(returnInstruction));
    }
    tmpBlockInstruction->pushInstructionBack(std::move(ifInstruction));
    return std::move(tmpBlockInstruction);
}

template<typename TPtrNode, typename TPtrAction>
std::unique_ptr<BlockInstruction> Compiler::prepareBaseInstructions(
    const std::shared_ptr<Graph>& graph,
    std::vector<TPtrAction>& actions,
    const TPtrNode& toNode,
    bool applyEdgeMode,
    bool simpleApplyEdgeMode)
{
    const std::string stateTo = toNode->getName();
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    if (!actions.empty() && actions.back()->getType() == ActionType::Assignment &&
        actions.back()->getLeftSide() == "player")
    {
        if (applyEdgeMode)
        {
            blockInstruction->pushInstructionBack(
                std::make_unique<CustomInstruction>("currentState = " + std::to_string(graph->getNodeId(stateTo))));
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("return true"));
        }
        else
        {
            if (verification_)
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "verificationCache.insert(std::make_pair(mr," + std::to_string(graph->getNodeId(stateTo)) +
                            ")).second",
                        ComparisonType::Neg));
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("abort()"));
                blockInstruction->pushInstructionBack(std::move(ifInstruction));
            }
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("moves.push_back(mr)"));
            actions.pop_back();
        }
    }
    else
    {
        std::string stateFunctionArguments = "moves, mr";
        std::string stateName = "state_";

        if (applyEdgeMode)
        {
            stateFunctionArguments = "mr";
            stateName = "apply_state_";
        }

        if (!optNoCycleDetection_)
        {
            stateFunctionArguments += "," + mainCacheName_;
        }

        if (const auto binding = toNode->getBinding())
        {
            if (simpleApplyEdgeMode)
            {
                stateFunctionArguments += "," + getVariableValueFromTagString(toNode);
            }
            else
            {
                stateFunctionArguments += "," + binding->getVariableName();
            }
        }

        std::string functionName = std::to_string(graph->getNodeId(stateTo));

        if (preserveOriginalNames_)
        {
            functionName = stateTo;
        }

        if (applyEdgeMode)
        {
            if (simpleApplyEdgeMode)
            {
                blockInstruction->pushInstructionBack(
                    std::make_unique<ReturnInstruction>(stateName + functionName + "(" + stateFunctionArguments + ")"));
            }
            else
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        stateName + functionName + "(" + stateFunctionArguments + ")"));
                ifInstruction->addInstruction(std::move(std::make_unique<ReturnInstruction>("true")));
                blockInstruction->pushInstructionBack(std::move(ifInstruction));
            }
        }
        else
        {
            blockInstruction->pushInstructionBack(
                std::make_unique<CustomInstruction>(stateName + functionName + "(" + stateFunctionArguments + ")"));
        }
    }

    return blockInstruction;
}

std::unique_ptr<BlockInstruction> Compiler::generateVoidEdgeInstruction(
    const std::shared_ptr<Graph>& graph,
    const std::shared_ptr<Edge>& edge,
    int iid,
    bool applyEdgeMode,
    bool addReturn,
    bool skipFirstInstruction)
{
    const std::string stateFrom = edge->fromName();
    const std::string stateTo = edge->toName();
    const auto& baseActions = graph->getEdge(stateFrom, stateTo, iid)->getActions();
    std::vector<std::shared_ptr<IAction>> actions(baseActions.begin(), baseActions.end());
    std::unique_ptr<BlockInstruction> blockInstruction =
        prepareBaseInstructions(graph, actions, edge->getRightNode(), applyEdgeMode);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);
    auto skipFirstInstructionIter = actions.rend();
    if (skipFirstInstruction)
    {
        skipFirstInstructionIter--;
    }

    std::vector<std::shared_ptr<Node>> nodes = {edge->getLeftNode()};
    nodes.insert(nodes.end(), edge->getInnerNodes().begin(), edge->getInnerNodes().end());
    assert(nodes.size() == baseActions.size());
    nodes.push_back(edge->getRightNode());
    if (baseActions.back()->getType() == ActionType::Assignment && baseActions.back()->getLeftSide() == "player")
    {
        // last action was erased in prepareBaseInstructions call, therefore we need to adjust set of nodes
        nodes.pop_back();
    }
    std::reverse(nodes.begin(), nodes.end());
    auto nodeIt = nodes.begin();
    for (auto action_iterator = actions.rbegin(); action_iterator != actions.rend(); action_iterator++)
    {
        auto action = *action_iterator;
        nodeIt++;

        if (action_iterator == skipFirstInstructionIter)
        {
            break;
        }

        if (action->getType() == ActionType::Assignment)
        {
            std::string lvalue = action->getLeftSide();
            blockInstruction->pushInstructionFront(
                std::make_unique<AssignmentInstruction>(lvalue, action->getRightSide()));
            blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
                getTemporaryVariableName(temporaryVariableCnt, edgeId), action->getLeftSide(), "const auto"));
            blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
                lvalue, getTemporaryVariableName(temporaryVariableCnt, edgeId)));
            temporaryVariableCnt++;
        }
        else if (action->getType() == ActionType::Comparison)
        {
            const auto cmpType = action->getNegated() ? ComparisonType::Neq : ComparisonType::Eq;
            std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>(action->getLeftSide(), action->getRightSide(), cmpType));

            ifInstruction->addInstruction(std::move(blockInstruction));

            if (addReturn)
            {
                if (applyEdgeMode)
                {
                    ifInstruction->addInstruction(std::make_unique<CustomInstruction>("return false;"));
                }
                else
                {
                    ifInstruction->addInstruction(std::make_unique<CustomInstruction>("return"));
                }
            }
            blockInstruction = std::make_unique<BlockInstruction>();
            blockInstruction->pushInstructionBack(std::move(ifInstruction));
        }
        else if (action->getType() == ActionType::Tag)
        {
            const auto tagValueStr = getTagValueString(action, edge);
            if (applyEdgeMode)
            {
                blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>("currentMrId++"));
                std::unique_ptr<IfInstruction> ifInstruction;
                // TODO: this is an optimization for move application (extracting value of node generator parameter
                //       from move vector instead of iterating over all values), but does not work for some games
                // const auto binding = edge->getRightNode()->getBinding();
                // if (binding && binding->getVariableName() == action->toString())
                // {
                //     const auto [minValue, maxValue] = valueAssigner_.getRangeValueForTag(binding->toTagStringId());
                //     const auto tagInRangeExpression = getValueInRangeExpressionString("mr[currentMrId]", minValue, maxValue);
                //     ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                //         "static_cast<int>(mr.size()) > currentMrId && " + tagInRangeExpression));

                //     blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
                //         binding->getVariableName(), "mr[currentMrId] - " + std::to_string(minValue), "const auto"));
                // }
                // else
                // {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size()) > currentMrId && mr[currentMrId] == " + tagValueStr));
                // }
                blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("currentMrId--"));
                ifInstruction->addInstruction(std::move(blockInstruction));
                blockInstruction = std::make_unique<BlockInstruction>();
                blockInstruction->pushInstructionBack(std::move(ifInstruction));
            }
            else
            {
                bool useArray =
                    graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
                if (useArray)
                {
                    int tagPosition =
                        graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->getTagPositionForNode(
                            (*nodeIt)->getName());
                    assert(tagPosition != -1);
                    const auto accessTag = "mr[" + std::to_string(tagPosition) + "]";
                    const auto setTag = accessTag + " = " + tagValueStr;
                    const auto unsetTag = accessTag + " = " + UNUSED_TAG_VALUE;
                    blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(setTag));
                    blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(unsetTag));
                }
                else
                {
                    const auto pushTag = "mr.push_back(" + tagValueStr + ")";
                    blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(pushTag));
                    blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("mr.pop_back()"));
                }
            }
        }
        else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
        {
            std::unique_ptr<CustomInstruction> returnInstruction = nullptr;
            if (addReturn)
            {
                if (applyEdgeMode)
                {
                    returnInstruction = std::make_unique<CustomInstruction>("return false");
                }
                else
                {
                    returnInstruction = std::make_unique<CustomInstruction>("return");
                }
            }
            blockInstruction = addActionPattern(
                action, graph, stateFrom, stateTo, iid, std::move(blockInstruction), std::move(returnInstruction));
        }
    }

    return wrapIntoLoopIfNeeded(edge, std::move(blockInstruction));
}

std::unique_ptr<BlockInstruction> Compiler::prepareBaseInstructions(
    const std::shared_ptr<Graph>& graph,
    const std::vector<std::shared_ptr<IAction>>& actions,
    const std::string& patternFrom,
    const std::string& patternTo,
    int edgeIdx,
    const std::shared_ptr<Edge>& edge,
    int iid,
    const std::string& prefix,
    int patternId)
{
    const std::string& stateFrom = edge->fromName();
    const std::string& stateTo = edge->toName();
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    std::string functionArguments;
    if (!optNoCycleDetection_)
    {
        // bool bSkipStateCache = areAllNodesInPatternGraphUnique_.count({patternFrom, patternTo});
        // if (patternId == 2)
        // {
        //     bSkipStateCache = areAllNodesInApplyAnyGraphUnique_.count({patternFrom, patternTo});
        // }
        functionArguments = mainCacheName_;
        // if (!bSkipStateCache)
        // {
        //     functionArguments += ", mr";
        // }
    }
    if (const auto binding = edge->getRightNode()->getBinding())
    {
        functionArguments += "," + binding->getVariableName();
    }
    std::string name = std::to_string(graph_->getNodeId(stateTo));
    if (preserveOriginalNames_)
    {
        name = stateTo;
    }
    std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
        std::make_unique<ComparisonInstruction>(prefix + name + "(" + functionArguments + ")"));

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
    const std::shared_ptr<Edge>& edge,
    int iid,
    int edgeIdx,
    const std::string& functionType)
{
    const std::string& stateFrom = edge->fromName();
    const std::string& stateTo = edge->toName();
    std::string prefix = "is_legal_" + functionType;

    if (preserveOriginalNames_)
    {
        prefix += from + "_" + to + "_";
    }
    else
    {
        prefix += std::to_string(graph_->getNodeId(from)) + "_" + std::to_string(graph_->getNodeId(to)) + "_";
    }

    bool bSkipStateCache = areAllNodesInPatternGraphUnique_.count({from, to});

    const auto& actions = graph->getEdge(stateFrom, stateTo, iid)->getActions();
    std::unique_ptr<BlockInstruction> blockInstruction =
        prepareBaseInstructions(graph, actions, from, to, edgeIdx, edge, iid, prefix, functionType.empty() ? 0 : 2);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);

    for (auto action_iterator = actions.rbegin(); action_iterator != actions.rend(); action_iterator++)
    {
        auto action = *action_iterator;

        if (action->getType() == ActionType::Assignment)
        {
            std::string lvalue = action->getLeftSide();
            blockInstruction->pushInstructionFront(
                std::make_unique<AssignmentInstruction>(lvalue, action->getRightSide()));
            blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
                getTemporaryVariableName(temporaryVariableCnt, edgeId), action->getLeftSide(), "const auto"));
            blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
                lvalue, getTemporaryVariableName(temporaryVariableCnt, edgeId)));
            temporaryVariableCnt++;
        }
        else if (action->getType() == ActionType::Comparison)
        {
            const auto cmpType = action->getNegated() ? ComparisonType::Neq : ComparisonType::Eq;
            std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>(action->getLeftSide(), action->getRightSide(), cmpType));

            ifInstruction->addInstruction(std::move(blockInstruction));
            blockInstruction = std::make_unique<BlockInstruction>();
            blockInstruction->pushInstructionBack(std::move(ifInstruction));
        }  // Ignore tags in patterns
        /*else if (action->getType() == ActionType::Tag && !bSkipStateCache)
        {
            const auto tagValueStr = getTagValueString(action, edge);
            std::string pushTag;
            bool useArray = graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
            if (useArray)
            {
                int tagPosition =
                    graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->getTagPositionForNode(stateFrom);
                assert(tagPosition != -1);
                pushTag = "mr[" + std::to_string(tagPosition) + "] = " + tagValueStr;
            }
            else
            {
                pushTag = "mr.push_back(" + tagValueStr + ")";
            }
            blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(pushTag));

            if (!useArray)
            {
                blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("mr.pop_back()"));
            }
        }*/
        else if (action->getType() == ActionType::Reachability || action->getType() == ActionType::PatternAny)
        {
            blockInstruction = addActionPattern(action, graph, stateFrom, stateTo, iid, std::move(blockInstruction));
        }
    }

    return wrapIntoLoopIfNeeded(edge, std::move(blockInstruction));
}

void Compiler::generateGetFromStateForEdge(const std::shared_ptr<Graph>& graph)
{
    auto function = std::make_unique<Function>("getFromStateForEdge", "int", "", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    auto sw = std::make_unique<SwitchInstruction>("val");

    const auto& edges = graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        const auto& actionBack = graph->getEdge(stateFrom, stateTo, edgeId)->getActions().back();
        const auto& actionFront = graph->getEdge(stateFrom, stateTo, edgeId)->getActions().front();
        std::string id = std::to_string(graph->getNodeId(stateTo));

        if (actionBack->getType() == ActionType::Assignment && actionBack->getLeftSide() == "player")
        {
            sw->addCaseInstruction(
                graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(std::make_unique<ReturnInstruction>(id)));
        }
        else if (actionBack->getType() == ActionType::Tag || stateFrom == "begin")
        {
            const auto path =
                graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getUnambiguousPathFromNode(stateTo, true);

            if (path.size())
            {
                id = std::to_string(graph->getNodeId(std::get<1>(path.back())));
            }
            sw->addCaseInstruction(
                graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(std::make_unique<ReturnInstruction>(id)));
        }
    }

    function->addInstruction(std::move(sw));
    function->addInstruction(std::make_unique<ReturnInstruction>("-1"));
    program_.addFunction(std::move(function));
}

void Compiler::generateGetStateDescription()
{
    auto function = std::make_unique<Function>("getStateDescription", "std::string", "", true, true);
    function->addInstruction(std::make_unique<VariableDeclarationInstruction>("ss", "std::stringstream"));

    for (const auto& var : program_.getVariables())
    {
        if (!parser_.isVariable(var->identifier))
        {
            continue;
        }
        function->addInstruction(std::make_unique<CustomInstruction>(
            "ss << \"" + var->identifier + " = \" << " +
            formatValueForPrinting(var->identifier, var->valueType, var->value.get(), valueAssigner_, parser_) +
            " << std::endl"));
    }
    function->addInstruction(
        std::make_unique<CustomInstruction>("ss << \"currentState = \" << currentState << std::endl"));
    function->addInstruction(
        std::make_unique<CustomInstruction>("ss << \"currentMrId = \" << currentMrId << std::endl"));
    function->addInstruction(std::make_unique<ReturnInstruction>("ss.str()"));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunStateFunction(const std::shared_ptr<Graph>& graph, bool applyMode)
{
    std::string functionName = "runState";
    std::string prefix = "state_";

    if (applyMode)
    {
        functionName = "runApplyState";
        prefix = "apply_state_";
    }

    auto function = std::make_unique<Function>(functionName, "void", "", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", "int"));

    if (applyMode)
    {
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("mr", "const move_representation&"));
    }
    else
    {
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
        function->addArgument(std::make_unique<VariableDeclarationInstruction>("mr", "move_representation&"));
    }
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
    std::string functionArguments = "moves, mr";
    if (applyMode)
    {
        functionArguments = "mr";
    }
    if (!optNoCycleDetection_)
    {
        functionArguments += "," + mainCacheName_;
    }

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (const auto& [edge, iid] :
         graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgesWithActionChangePlayer())
    {
        std::string stateName = std::to_string(graph->getNodeId(edge->toName()));
        if (preserveOriginalNames_)
        {
            stateName = edge->toName();
        }
        auto block = std::make_unique<BlockInstruction>();
        block->pushInstructionBack(
            std::make_unique<CustomInstruction>(prefix + stateName + "(" + functionArguments + ")"));
        block->pushInstructionBack(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getNodeId(edge->toName()), std::move(block));
    }

    std::string stateBeginName = std::to_string(graph->getNodeId("begin"));
    if (preserveOriginalNames_)
    {
        stateBeginName = "begin";
    }
    auto block = std::make_unique<BlockInstruction>();
    block->pushInstructionBack(
        std::make_unique<CustomInstruction>(prefix + stateBeginName + "(" + functionArguments + ")"));
    block->pushInstructionBack(std::make_unique<ReturnInstruction>());

    sw->addCaseInstruction(graph->getNodeId("begin"), std::move(block));

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateSpecialFunctions(const std::shared_ptr<Graph>& graph)
{
    generateRunStateFunction(graph, true);
    generateRunStateFunction(graph);
    generateGetFromStateForEdge(graph);
    generateGetStateDescription();

    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", "", true);
    isTerminal->addInstruction(
        std::make_unique<ReturnInstruction>("currentState == " + std::to_string(graph->getNodeId("end"))));

    auto getPlayerScore = std::make_unique<Function>("getPlayerScore", "Score", "", true);
    getPlayerScore->addArgument(std::make_unique<VariableDeclarationInstruction>("player", "Player"));
    getPlayerScore->addInstruction(std::make_unique<ReturnInstruction>("goals[player - 1]"));

    auto getCurrentPlayer = std::make_unique<Function>("getCurrentPlayer", "PlayerOrKeeper", "", true);
    getCurrentPlayer->addInstruction(std::make_unique<ReturnInstruction>("player"));

    auto getCurrentState = std::make_unique<Function>("getCurrentState", "std::string", "", true);
    getCurrentState->addInstruction(std::make_unique<ReturnInstruction>("std::to_string(currentState)"));

    auto getAllMovesFunction = std::make_unique<Function>("getAllMoves", "void", "", true);
    getAllMovesFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
    getAllMovesFunction->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, mainCacheType_ + "&"));
    std::string clearingCaches = mainCacheName_ + ".reset();\n";
    if (verification_)
    {
        clearingCaches += "verificationCache.clear();\n";
    }
    getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
        clearingCaches + "moves.clear();\nMove mr;\nrunState(currentState, moves,mr.mr," + mainCacheName_ + ")"));

    auto applyMoveFunction = std::make_unique<Function>("applyMove", "void", "", true);
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("m", "const Move&"));
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("rgCache", "RgCache&"));
    applyMoveFunction->addInstruction(std::make_unique<CustomInstruction>(
        mainCacheName_ + ".reset();" +
        R"(const move_representation &v = m.mr;
        currentMrId = 0;
    runApplyState(currentState, v, rgCache);
  )"));

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

    auto function = std::make_unique<Function>("applyAnyMove", "bool", "", true);
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
    bool cacheNeeded = false;
    std::string cacheName = "applyAnyCache";
    auto nodesForApplyAnyMove = getNodesForApplyAnyMoveFunction(applyAnyMoveGraphs_);
    for (const auto& [nodeName, nodesToPlayerChangeOrEnd] : nodesForApplyAnyMove)
    {
        auto block = std::make_unique<BlockInstruction>();
        for (const auto& nodeTo : nodesToPlayerChangeOrEnd)
        {
            std::string functionArguments;
            if (!optNoCycleDetection_)
            {
                // std::string cacheDecl = containerChooser_.getCustomName({nodeName, nodeTo, 2});
                // if (containerChooser_.isInCache({nodeName, nodeTo, 2}))
                // {
                //     cacheDecl += "&" + cacheName + "=" + mainCacheName_ + "." +
                //                  containerChooser_.getFromCache({nodeName, nodeTo, 2});
                //     cacheDecl += ";\n";
                //     //+ cacheName + ".reset(" + containerChooser_.getType({nodeName, nodeTo, 2}) + ")";
                // }
                // else
                // {
                //     cacheDecl += " " + cacheName;
                // }
                bool skipStateCache = areAllNodesInApplyAnyGraphUnique_.count({nodeName, nodeTo});
                functionArguments += mainCacheName_;
                cacheNeeded |= !skipStateCache;
            }

            std::string functionName = std::to_string(graph_->getNodeId(nodeName)) + "_" +
                                       std::to_string(graph_->getNodeId(nodeTo)) + "_" +
                                       std::to_string(graph_->getNodeId(nodeName));
            if (preserveOriginalNames_)
            {
                functionName = nodeName + "_" + nodeTo + "_" + nodeName;
            }
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "is_legal_any2_" + functionName + "(" + functionArguments + ")"));
            ifInstruction->addInstruction(
                std::make_unique<AssignmentInstruction>("currentState", std::to_string(graph_->getNodeId(nodeTo))));
            ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("true"));

            block->pushInstructionBack(std::move(ifInstruction));
            if (cacheNeeded)
            {
                block->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".reset()"));
            }
        }
        block->pushInstructionBack(std::make_unique<ReturnInstruction>("false"));
        sw->addCaseInstruction(graph_->getNodeId(nodeName), std::move(block));
    }

    if (cacheNeeded)
    {
        auto tmpBlockInstruction = std::make_unique<BlockInstruction>();
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".reset()"));
        function->addInstruction(std::move(tmpBlockInstruction));
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
        bool skipCache = false;
        if (patternId == 2)
        {
            skipCache = areAllNodesInApplyAnyGraphUnique_.count({from, to});
        }
        else
        {
            skipCache = areAllNodesInPatternGraphUnique_.count({from, to});
        }
        generateBoolStateFunctions(from, to, graph, patternId, skipCache);
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
    generateVoidStateFunctions(graph_);
    generateVoidStateFunctions(graph_, true);
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

std::unique_ptr<BlockInstruction> Compiler::wrapIntoLoopIfNeeded(
    const std::shared_ptr<Edge>& edge, std::unique_ptr<BlockInstruction> blockInstruction) const
{
    const auto leftBinding = edge->getLeftNode()->getBinding();
    const auto rightBinding = edge->getRightNode()->getBinding();
    if (rightBinding && leftBinding != rightBinding)
    {
        // Optimization: not create loops when only one value will be accepted
        const auto ifInstruction = dynamic_cast<IfInstruction*>(blockInstruction->frontInstruction().get());
        if (ifInstruction != nullptr && ifInstruction->getCondition()->getType() == ComparisonType::Eq)
        {
            const auto [lhs, rhs] = ifInstruction->getCondition()->getSubexpressions();
            const auto bindParameterName = rightBinding->getVariableName();
            if (lhs == bindParameterName)
            {
                auto instructionsInsideIf = ifInstruction->extractInstructions();
                blockInstruction->popInstructionFront();
                for (auto& insideInstruction : std::ranges::reverse_view(instructionsInsideIf))
                {
                    blockInstruction->pushInstructionFront(std::move(insideInstruction));
                }
                blockInstruction->pushInstructionFront(
                    std::make_unique<AssignmentInstruction>(bindParameterName, rhs, "const auto"));
                return blockInstruction;
            }
            if (rhs == bindParameterName)
            {
                auto instructionsInsideIf = ifInstruction->extractInstructions();
                blockInstruction->popInstructionFront();
                for (auto& insideInstruction : std::ranges::reverse_view(instructionsInsideIf))
                {
                    blockInstruction->pushInstructionFront(std::move(insideInstruction));
                }
                blockInstruction->pushInstructionFront(
                    std::make_unique<AssignmentInstruction>(bindParameterName, lhs, "const auto"));
                return blockInstruction;
            }
        }

        const LoopFactory loopFactory(parser_, valueAssigner_);
        auto loopInstruction = loopFactory.createLoopInstruction(*rightBinding);
        loopInstruction->addInstruction(std::move(blockInstruction));
        auto result = std::make_unique<BlockInstruction>();
        result->pushInstructionBack(std::move(loopInstruction));
        return result;
    }

    return blockInstruction;
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

std::string Compiler::getTagValueString(const std::shared_ptr<IAction>& action, const std::shared_ptr<Edge>& edge)
{
    const auto leftBinding = edge->getLeftNode()->getBinding();
    if (leftBinding && leftBinding->getVariableName() == action->toString())
    {
        const auto tagName = leftBinding->toTagStringId();
        return std::to_string(valueAssigner_.getBaseValueForTag(tagName)) + " + " + leftBinding->getVariableName();
    }
    const auto rightBinding = edge->getRightNode()->getBinding();
    if (rightBinding && rightBinding->getVariableName() == action->toString())
    {
        const auto tagName = rightBinding->toTagStringId();
        return std::to_string(valueAssigner_.getBaseValueForTag(tagName)) + " + " + rightBinding->getVariableName();
    }
    const auto tagName = action->toString();
    return std::to_string(valueAssigner_.getBaseValueForTag(tagName));
}

const std::shared_ptr<IStateCache>& Compiler::getStateCacheSafe(const std::string& state) const
{
    const auto it = stateToCache_.find(state);
    if (it == stateToCache_.end())
    {
        throw std::invalid_argument("[Compiler] Unknown cache for state " + state);
    }
    return it->second;
}

template<typename TPtrNode>
std::string Compiler::getVariableValueFromTagString(const TPtrNode& node) const
{
    const auto binding = node->getBinding();
    assert(binding);
    return "mr[currentMrId-1] -" + std::to_string(valueAssigner_.getBaseValueForTag(binding->toTagStringId()));
}
