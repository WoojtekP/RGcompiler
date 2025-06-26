#include <functional>
#include <ranges>

#include <common/Common.hpp>
#include <compiler/Compiler.hpp>
#include <compiler/stateCache/IStateCache.hpp>
#include <compiler/stateCache/StateCacheFactory.hpp>
#include <parser/Parser.hpp>
#include <printer/Printer.hpp>
#include <program/LoopFactory.hpp>

namespace
{
constexpr const int SMALL_VECTOR_MOVE_SIZE = 12;
const std::string UNUSED_TAG_VALUE = "-1";
constexpr std::string_view CUSTOM_TYPE_WORD = "int";
constexpr std::string_view CURRENT_MR_ID_WORD = "currentMrId";
constexpr std::string_view APPLY_STATE_WORD = "apply_state_";
constexpr std::string_view STATE_WORD = "state_";
constexpr std::string_view IS_LEGAL_WORD = "is_legal_";
constexpr std::string_view CURRENT_STATE_WORD = "currentState";
constexpr std::string_view APPLY_ANY_WORD = "apply_any_";

void fillPaternsDependency(
    const std::shared_ptr<Graph> graph,
    std::map<int, std::set<int>>& patternsDependency,
    const std::map<std::pair<std::string, std::string>, int>& patternNameToId,
    int id)
{
    for (const auto& [edge, iid] : graph->getAllEdges())
    {
        for (const auto& action : edge->getActions())
        {
            if (action->getType() == ActionType::Reachability)
            {
                const std::string from = action->getLeftSide();
                const std::string to = action->getRightSide();
                int newId = patternNameToId.at({from, to});
                patternsDependency[id].insert(newId);
                patternsDependency[newId].insert(id);
            }
        }
    }
}

void visitAdjacentPatterns(
    int patternId, std::map<int, std::set<int>>& patternsDependency, std::set<int>& patternsWithCacheResults)
{
    if (!patternsWithCacheResults.insert(patternId).second)
    {
        return;
    }

    for (int newId : patternsDependency[patternId])
    {
        visitAdjacentPatterns(newId, patternsDependency, patternsWithCacheResults);
    }
}

std::unique_ptr<IInstruction> debugInstruction(std::string functionName)
{
    std::string information = "In function: " + functionName + "\\n";
    return std::make_unique<CustomInstruction>("std::cout << \"" + information + "\"");
}

bool nodeInThisEdge(const std::shared_ptr<Edge>& edge, const std::string& nodeName)
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

std::optional<std::string> getTagType(const std::string& tag)
{
    assert(tag.size());
    std::string tagTmp = tag.substr(1, tag.size());
    auto pos = tagTmp.find(":");

    if (pos != std::string::npos)
    {
        std::string res = tagTmp.substr(pos + 2);
        res.pop_back();
        return res;
    }
    return {};
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

void optimizePopPushSequences(const std::unique_ptr<Function>& function)
{
    const auto& instructions = function->getInstructions();
    if (instructions.size() < 2)
    {
        return;
    }
    auto prevBlockInstr = dynamic_cast<BlockInstruction*>(instructions.front().get());
    for (auto instr = instructions.begin() + 1; instr != instructions.end(); ++instr)
    {
        auto currBlockInstr = dynamic_cast<BlockInstruction*>(instr->get());
        if (prevBlockInstr == nullptr || currBlockInstr == nullptr)
        {
            continue;
        }
        const auto& lastInstrStr = prevBlockInstr->backInstruction()->toString(0, 0, false);
        const auto& firstInstrStr = currBlockInstr->frontInstruction()->toString(0, 0, false);
        if (lastInstrStr == "mr.pop_back()" && firstInstrStr.starts_with("mr.emplace_back"))
        {
            prevBlockInstr->popInstructionBack();
            currBlockInstr->popInstructionFront();
            const auto prefixLen = std::strlen("mr.emplace_back(");
            const auto pushedValue = firstInstrStr.substr(prefixLen, firstInstrStr.size() - 1 - prefixLen);
            currBlockInstr->pushInstructionFront(std::make_unique<AssignmentInstruction>("mr.back()", pushedValue));
        }
        prevBlockInstr = currBlockInstr;
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
, allUnique_(options.allUnique_)
, optGccInline_(static_cast<InlineMode>(options.gccInline_))
, maxMoveLen_(options.maxMoveLen_ == -1 ? std::nullopt : std::optional(options.maxMoveLen_))
, temporaryVariableNamePrefix_("old")
, optNoCycleDetection_(options.noCycleDetection_)
, mainCacheName_("rgCache")
, mainCacheType_("RgCache")
, containerChooser_(mainCacheType_)
, graphOperatorManager_(std::make_shared<GraphOperatorManager>())

{
    assert(options.gccInline_ >= 0);
    assert(options.gccInline_ <= 2);
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
            data.insert(edge["identifier"].get<std::string>());
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
            areAllNodesInApplyAnyGraphUnique_.insert(from);
        }
    }
}

void Compiler::initializePragmaRepeat()
{
    if (allUnique_)
    {
        return;
    }

    pragmaRepeatFlatData_.parse(parser_);

    pragmaRepeatFlatData_.initializeDataForGraphs(patternReachabilityGraphs_, graph_, 0);
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
    for (const auto& [nodeName, variables] : pragmaRepeatFlatData_.getStateToIdentifiersMap())
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
            std::make_shared<Node>(edge["lhs"]),
            std::make_shared<Node>(edge["rhs"]),
            std::vector<std::shared_ptr<IAction>> {actionFactory.createAction(edge["label"])}));
    }

    graph_->initialize(valueAssigner_);
    mainGraph_ = graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forMainGraph();
    mainGraph_->initialize(valueAssigner_);

    patternReachabilityGraphs_ =
        graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forPatterns(ActionType::Reachability);
    applyAnyMoveGraphs_ = graphOperatorManager_->getOperator<GenerateGraphsOperator>(graph_)->forApplyAnyMove();

    if (optConditionsSimplePathCompression_)
    {
        unoptimizedGraph_ =
            graphOperatorManager_->getOperator<GetOptimizedGraphOperator>(graph_)->getGraphWithOptimizedPaths(
                valueAssigner_);
        std::swap(unoptimizedGraph_, graph_);
    }
    else
    {
        unoptimizedGraph_ = graph_;
    }

    initializePatternGraphs(patternReachabilityGraphs_, optConditionsSimplePathCompression_);
    initializePatternGraphs(applyAnyMoveGraphs_);
}

void Compiler::initializePatternGraphs(
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& patterns, bool isSimplePath)
{
    for (const auto& [from, to, graph] : patterns)
    {
        graph->initialize(valueAssigner_);
    }

    if (isSimplePath)
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

void Compiler::initializePatternGraphs(
    std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>>& patterns, bool isSimplePath)
{
    for (const auto& [from, to, graph] : patterns)
    {
        graph->initialize(valueAssigner_);
    }

    if (isSimplePath)
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
    if (maxMoveLen_)
    {
        return {"boost::container::static_vector", *maxMoveLen_};
    }
    if (graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition())
    {
        return {"std::array", containerSize};
    }
    if (containerSize != -1)
    {
        return {"boost::container::static_vector", containerSize};
    }
    const auto maxIndexFromPragma = graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->maxDefinedIndex();
    return {"boost::container::small_vector", std::max(SMALL_VECTOR_MOVE_SIZE, maxIndexFromPragma + 1)};
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
    printer.printVariables(program_.getVariables());
    printer.printHashAndComparisonFunctions(parser_.getVariables());
    printer.printFunctions(program_.getFunctions());
    printer.endMainClass();
    printer.printMainCache(containerChooser_.getAdditionalData(stateToCache_, arePatternAndMainGraphUnique()));
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

    auto playerCountConstantType = std::make_shared<CustomType>(std::string(CUSTOM_TYPE_WORD));
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

    const std::string initialState = std::to_string(graph->getNodeId(common::BEGIN_WORD));
    auto currentStateType = std::make_shared<CustomType>(std::string(CUSTOM_TYPE_WORD));
    auto currentStateValue = std::make_unique<SingleValue>(initialState);
    program_.addVariableDeclaration(
        std::make_unique<Variable>("currentState", std::move(currentStateType), std::move(currentStateValue)));

    if (verification_)
    {
        program_.addVariableDeclaration(std::make_unique<Variable>(
            "verificationCache",
            std::move(std::make_shared<CustomType>("std::unordered_map<move_representation, int, move_hash>"))));
    }
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
        if (action->getType() == ActionType::Assignment)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(lvalue, getTemporaryVariableName(cnt, edgeId)));
            cnt++;
        }
        else if (action->getType() == ActionType::AssignmentAny)
        {
            function->addInstruction(
                std::make_unique<AssignmentInstruction>(lvalue, getTemporaryVariableName(cnt, edgeId)));
            cnt++;
        }
        else
        {
            throw std::invalid_argument("[Compiler] Cannot restore assignment from action: " + action->toString());
        }
    }
}

std::string Compiler::getTemporaryVariableName(int idx, int edgeId)
{
    return temporaryVariableNamePrefix_ + std::to_string(edgeId) + "_" + std::to_string(idx);
}

void Compiler::generateVoidStateFunctions(const std::shared_ptr<Graph>& graph, bool applyMode)
{
    for (auto& node : graph->getOuterNodes())
    {
        if (!mainGraph_->getNodeIdOptional(node->getName()))
        {
            continue;
        }
        const std::string state = node->toString();
        std::string prefix(STATE_WORD);
        bool isSimpleApply = false;
        if (applyMode)
        {
            prefix = std::string(APPLY_STATE_WORD);
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

        std::string attributes;
        if (optGccInline_ == InlineMode::UniqueOnly && pragmaUniqueData_.count(state))
        {
            attributes += "__attribute__((always_inline))inline";
        }
        std::unique_ptr<Function> function = std::make_unique<Function>(functionName, functionType, attributes);

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
        bool isCacheNeed = checkIsCacheNeed(functionName, graph);
        if (!optNoCycleDetection_ && isCacheNeed)
        {
            function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
        }

        if (printOriginalNames_)
        {
            function->addInstruction(debugInstruction(prefix + state));
        }

        if (pragmaRepeatFlatData_.getStateToIdentifiersMap().count(state))
        {
            const auto& stateCache = getStateCacheSafe(state);
            const std::string cacheVarName = "cache";
            function->addInstruction(std::make_unique<AssignmentInstruction>(
                cacheVarName, mainCacheName_ + "." + stateCache->getCacheName(), "auto&"));

            auto testCacheInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>(cacheVarName + stateCache->getTestInstruction()));
            addReturnInstruction(testCacheInstruction, applyMode);
            function->addInstruction(std::move(testCacheInstruction));

            const auto insertInstruction = cacheVarName + stateCache->getInsertInstruction();
            function->addInstruction(std::make_unique<CustomInstruction>(insertInstruction));
        }
        else if (!(allUnique_ || pragmaUniqueData_.count(node->getName())))
        {
            auto nodeId = std::to_string(graph_->getNodeId(state));
            const std::string cacheData = "*this, mr, " + nodeId;
            std::unique_ptr<IfInstruction> ifInstruction =
                std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    mainCacheName_ + ".insert(" + cacheData + ")", ComparisonType::Neg));
            addReturnInstruction(ifInstruction, applyMode);
            function->addInstruction(std::move(ifInstruction));
        }

        bool skipForExhaustiveSimpleApply = false;
        if (applyMode && graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                             ->isMainSimpleApply(node->getName()))
        {
            auto edgeInstruction = generateVoidEdgeInstruction(
                node,
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->getActionListToTags(node),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->getActionListToPlayerChange(node),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->isExhaustive(node->getName()),
                graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                    ->hasAnyEmptyTagSequence(node->getName()),
                isCacheNeed);

            if (edgeInstruction)
            {
                function->addInstruction(std::move(edgeInstruction));

                skipForExhaustiveSimpleApply =
                    graphOperatorManager_->getOperator<PragmaSimpleApplyOperator>(unoptimizedGraph_)
                        ->isExhaustive(node->getName());
            }
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
                        function->addInstruction(generateVoidEdgeInstruction(
                            graph,
                            outgoingEdge,
                            iid,
                            applyMode,
                            /*addReturn*/ false,
                            /*skipFirstInstruction*/ false,
                            isCacheNeed));
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
                                disjointExhaustive && ++cnt == vectorOfNodeNames.size(),
                                isCacheNeed));
                        }
                    }
                }
            }
            else
            {
                for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
                {
                    function->addInstruction(generateVoidEdgeInstruction(
                        graph,
                        outgoingEdge,
                        iid,
                        applyMode,
                        /*addReturn*/ false,
                        /*skipFirstInstruction*/ false,
                        isCacheNeed));
                }
            }
        }

        if (applyMode)
        {
            function->addInstruction(std::move(std::make_unique<ReturnInstruction>("false")));
        }

        optimizePopPushSequences(function);
        program_.addFunction(std::move(function));
    }
}

std::unique_ptr<BlockInstruction> Compiler::getAssignments(
    const std::vector<std::shared_ptr<IAction>>& actions,
    const std::vector<std::string>& tags,
    std::vector<int>& minValues,
    const std::vector<int>& positions) const
{
    int curentPos = 1;
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();
    std::map<std::string, std::string> tagToValue;
    bool skipCurrentMrId = false;
    // !maxMoveLen_ && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();

    for (auto tag : tags)
    {
        auto tagVar = getTagVar(tag);
        if (tagVar)
        {
            std::string pos = mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + " - " +
                              std::to_string(minValues.size() - curentPos + 1);
            if (skipCurrentMrId)
            {
                assert(positions.size() > curentPos - 1);
                pos = std::to_string(positions[curentPos - 1]);
            }
            tagToValue[*tagVar] = "mr[" + pos + "] - " + std::to_string(minValues[curentPos - 1]);
            std::string staticCast = "static_cast<" + *getTagType(tag) + ">(" + *tagVar + ")";
            tagToValue[staticCast] = tagToValue[*tagVar];
        }
        curentPos++;
    }

    for (const auto& action : actions)
    {
        std::string actionStr = action->getRightSide();

        if (tagToValue.contains(actionStr))
        {
            actionStr = tagToValue[actionStr];
        }
        blockInstruction->pushInstructionBack(
            std::make_unique<AssignmentInstruction>(action->getLeftSide(), actionStr));
    }

    return std::move(blockInstruction);
}

std::unique_ptr<BlockInstruction> Compiler::makeSwitchForTags(
    const std::shared_ptr<SimpleApplySwitchTreeNode>& listOfActionsToTags,
    std::vector<std::string>& tags,
    int depth,
    const bool isExhaustive,
    const bool hasAnyEmptyTagSequence,
    std::vector<int>& minValues,
    std::vector<int>& positions,
    std::shared_ptr<Node> node,
    bool isCacheNeed)
{
    if (listOfActionsToTags->children_.empty())
    {
        std::unique_ptr<BlockInstruction> blockInstructionTmp =
            getAssignments(listOfActionsToTags->listOfActions_, tags, minValues, positions);

        blockInstructionTmp->pushInstructionBack(prepareBaseInstructions(
            unoptimizedGraph_,
            listOfActionsToTags->listOfActions_,
            listOfActionsToTags->endNode_,
            true,
            true,
            isCacheNeed));

        return std::move(blockInstructionTmp);
    }

    bool useArray =
        !maxMoveLen_ && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
    // Temporary removed usage of direct positions in move vector because of problems with simple apply in oware
    // In order for this to work whole semantic of simple apply would need to be changed to:
    // @simpleApply [tag1, tag2, /] -- to się aplikuje jak dwa tagi pasują i więcej tagów nie ma.
    // @simpleApply [tag1, tag2] -- to się aplikuje jak dwa tagi pasują, dalej mogą też być tagi.
    // @simpleApply [] -- to zawsze pasuje, ale każdy inny simpleApply ma pierwszeństwo bo ma dłuższy ciąg tagów.
    std::string pos = mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "++";
    // if (useArray)
    // {
    //     if (positions.empty())
    //     {
    //         // We calculate position of first tag in move vector.
    //         // Then each next tag will need to have position equal to last position + 1
    //         const std::string& tag = listOfActionsToTags->children_.begin()->first.tag_;
    //         const auto [lastNode, pos] =
    //             graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->getPositions(node, tag);
    //         positions.push_back(pos);
    //     }

    //     if (positions.size() == depth - 1)
    //     {
    //         positions.push_back(positions.back() + 1);
    //     }
    //     pos = std::to_string(positions[depth - 1]);
    // }

    auto sw = std::make_unique<SwitchInstruction>("mr[" + pos + "]");
    std::set<std::pair<int, int>> caseAlreadyAdded;
    int cnt = 0;
    for (auto pairFullTagAndChild : listOfActionsToTags->children_)
    {
        std::string tag = pairFullTagAndChild.first.tag_;
        std::pair<int, int> values;
        if (auto tagType = getTagType(tag))
        {
            values = valueAssigner_.getRangeValueForTag(*tagType);
        }
        else
        {
            values = valueAssigner_.getRangeValueForTag(tag);
        }
        int minValue = values.first, maxValue = values.second;
        minValues.push_back(minValue);
        tags.push_back(pairFullTagAndChild.first.tag_);
        auto innerInstructions = std::move(makeSwitchForTags(
            pairFullTagAndChild.second,
            tags,
            depth + 1,
            isExhaustive,
            hasAnyEmptyTagSequence,
            minValues,
            positions,
            node,
            isCacheNeed));
        if (!innerInstructions)
        {
            return nullptr;
        }
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
                std::unique_ptr<IfInstruction> ifInstruction;
                if (useArray)
                {
                    // ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    //     "mr[" + std::to_string(positions[depth]) + "]", "-1", ComparisonType::Neq));
                    ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "mr[" + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "]",
                        "-1",
                        ComparisonType::Neq));
                }
                else
                {
                    ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "static_cast<int>(mr.size()) >" + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD)));
                }

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
            if (!caseAlreadyAdded.insert(std::make_pair(minValue, maxValue)).second)
            {
                return nullptr;
            }
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
    const std::pair<std::vector<std::shared_ptr<IAction>>, std::unique_ptr<Node>>&
        listOfActionsToPlayerChangeAndEndNode,
    const bool isExhaustive,
    const bool hasAnyEmptyTagSequence,
    const bool isCacheNeed)
{
    static int nameCnt = 0;
    std::string functionName = "switch_" + std::to_string(nameCnt++);
    std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]]const move_representation&"));

    if (!optNoCycleDetection_ && isCacheNeed)
    {
        function->addArgument(std::make_unique<VariableDeclarationInstruction>(
            mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
    }

    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();
    if (!listOfActionsToTags->empty())
    {
        std::vector<int> minValues;
        std::unique_ptr<IInstruction> blockAction;
        std::vector<std::string> tags;
        std::vector<int> positions;

        auto switchBody = makeSwitchForTags(
            listOfActionsToTags,
            tags,
            1,
            isExhaustive,
            hasAnyEmptyTagSequence,
            minValues,
            positions,
            node,
            isCacheNeed);

        if (!switchBody)
        {
            return nullptr;
        }

        if (isExhaustive && !hasAnyEmptyTagSequence)
        {
            blockAction = std::move(switchBody);
        }
        else
        {
            std::unique_ptr<IfInstruction> ifInstruction;

            bool useArray = !maxMoveLen_ &&
                            graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
            if (useArray)
            {
                // ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                //     " mr[" + std::to_string(positions[0]) + "]", "-1", ComparisonType::Neq));
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    " mr[" + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "]", "-1", ComparisonType::Neq));
            }
            else
            {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size())",
                    mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD),
                    ComparisonType::Gr));
            }
            ifInstruction->addInstruction(std::move(switchBody));
            blockAction = std::move(ifInstruction);
        }

        blockInstruction->pushInstructionFront(std::move(blockAction));
    }

    bool useArray =
        !maxMoveLen_ && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
    if (!listOfActionsToPlayerChangeAndEndNode.first.empty())
    {
        std::vector<std::shared_ptr<IAction>> actonsToPlayerChangeAndEndNode =
            listOfActionsToPlayerChangeAndEndNode.first;
        std::vector<int> minValuesEmpty;
        std::unique_ptr<BlockInstruction> blockInstructionTmp =
            getAssignments(actonsToPlayerChangeAndEndNode, {}, minValuesEmpty, {});

        blockInstructionTmp->pushInstructionBack(prepareBaseInstructions(
            unoptimizedGraph_,
            actonsToPlayerChangeAndEndNode,
            listOfActionsToPlayerChangeAndEndNode.second,
            true,
            isCacheNeed));

        std::unique_ptr<IfInstruction> ifInstruction;
        if (!useArray)
        {
            ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                "static_cast<int>(mr.size())",
                mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD),
                ComparisonType::Neq));
        }
        else
        {
            ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                " mr[" + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "]", "-1", ComparisonType::Neq));
        }
        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
        blockInstruction->pushInstructionBack(std::move(ifInstruction));

        blockInstruction->pushInstructionBack(std::move(blockInstructionTmp));
    }

    if ((!isExhaustive || hasAnyEmptyTagSequence) && !useArray)
    {
        blockInstruction->pushInstructionFront(std::make_unique<AssignmentInstruction>(
            "const int tmp" + std::string(CURRENT_MR_ID_WORD), mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD)));
        blockInstruction->pushInstructionBack(std::make_unique<AssignmentInstruction>(
            mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD), "tmp" + std::string(CURRENT_MR_ID_WORD)));
    }

    function->addInstruction(std::move(blockInstruction));
    function->addInstruction(std::make_unique<ReturnInstruction>("false"));

    program_.addFunction(std::move(function));

    blockInstruction = std::make_unique<BlockInstruction>();
    std::string cacheName;
    if (isCacheNeed)
    {
        cacheName = "," + mainCacheName_;
    }
    const auto functionCall = functionName + "(mr " + cacheName + ")";
    functionCallCounter_[functionName]++;
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
    for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
    {
        function->addInstruction(generateVoidEdgeInstruction(graph, outgoingEdge, iid, applyMode));
    }
}

void Compiler::generateBoolStateFunctions(
    const std::string& from,
    const std::string& to,
    const std::shared_ptr<Graph>& graph,
    BoolFunctionType patternId,
    bool skipStateCache,
    const std::set<int>& finalNodes)
{
    std::string name(patternIdToPrefixName.at(patternId));
    std::string prefix = std::string(IS_LEGAL_WORD) + name;

    bool isApplyAnyMove = !finalNodes.empty();
    if (isApplyAnyMove)
    {
        prefix = std::string(APPLY_ANY_WORD);
    }

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
        bool isCacheNeed =
            (isApplyAnyMove ? checkIsCacheNeed(functionName, graph) : checkIsCacheNeedForPattern(from, to));

        if (isCacheNeed)
        {
            functionsWithCache_.insert(functionName);
        }

        if (printOriginalNames_)
        {
            function->addInstruction(debugInstruction(functionName));
        }

        if (!optNoCycleDetection_)
        {
            if (isCacheNeed)
            {
                function->addArgument(std::make_unique<VariableDeclarationInstruction>(
                    mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));
            }

            if (pragmaRepeatFlatData_.getStateToIdentifiersMap().count(state))
            {
                const auto& stateCache = getStateCacheSafe(state);
                const std::string cacheVarName = "cache";
                function->addInstruction(std::make_unique<AssignmentInstruction>(
                    cacheVarName, mainCacheName_ + "." + stateCache->getCacheName(), "auto&"));

                auto testCacheInstruction = std::make_unique<IfInstruction>(
                    std::make_unique<ComparisonInstruction>(cacheVarName + stateCache->getTestInstruction()));
                addReturnInstruction(testCacheInstruction, true);
                function->addInstruction(std::move(testCacheInstruction));

                const auto insertInstruction = cacheVarName + stateCache->getInsertInstruction();
                function->addInstruction(std::make_unique<CustomInstruction>(insertInstruction));
            }
            else if (!(pragmaUniqueData_.count(node->getName()) || allUnique_) && !skipStateCache)
            {
                auto nodeId = std::to_string(graph_->getNodeId(state));
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        mainCacheName_ + ".insert(*this, " + nodeId + ")", ComparisonType::Neg));
                ifInstruction->addInstruction(std::move(std::make_unique<ReturnInstruction>("false")));

                function->addInstruction(std::move(ifInstruction));
            }
        }

        const auto& outgoingEdges = graph->getOutgoingEdgesFrom(state);

        if (!isApplyAnyMove && state == to)
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else if (finalNodes.count(graph_->getNodeId(state)))
        {
            function->addInstruction(std::make_unique<ReturnInstruction>("true"));
        }
        else
        {
            if (isStateDisjoint(state))
            {
                handleBoolDisjoint(from, to, function, graph, state, isApplyAnyMove, isCacheNeed);
            }
            else
            {
                int edgeIdx = 0;
                for (auto& [outgoingEdge, iid] : outgoingEdges)
                {
                    function->addInstruction(generateBoolEdgeInstruction(
                        from,
                        to,
                        graph,
                        outgoingEdge,
                        iid,
                        edgeIdx++,
                        isApplyAnyMove,
                        /*addReturn*/ false,
                        /*skipFirstInstruction*/ false,
                        isCacheNeed));
                }
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
    std::unique_ptr<BlockInstruction> blockInstruction,
    std::unique_ptr<CustomInstruction> returnInstruction)
{
    std::string fromNode = std::to_string(graph_->getNodeId(action->getLeftSide()));
    std::string toNode = std::to_string(graph_->getNodeId(action->getRightSide()));
    std::string prefix(IS_LEGAL_WORD);
    std::string functionName = prefix + fromNode + "_" + toNode + "_" + fromNode;

    if (preserveOriginalNames_)
    {
        functionName = prefix + action->getLeftSide() + "_" + action->getRightSide() + "_" + action->getLeftSide();
    }

    std::string functionArguments;
    std::unique_ptr<BlockInstruction> tmpBlockInstruction = std::make_unique<BlockInstruction>();

    bool skipStateCache = areAllNodesInPatternGraphUnique_.count({action->getLeftSide(), action->getRightSide()});

    if (patternsWithCache_.count({action->getLeftSide(), action->getRightSide()}))
    {
        functionArguments += mainCacheName_;
    }

    const std::string functionCall = functionName + "(" + functionArguments + ")";
    functionCallCounter_[functionName]++;
    std::string comparisonExpression;
    if (!skipStateCache)
    {
        const auto functionResultVar = "result_" + std::to_string(graph->getEdgeId(stateFrom, stateTo, iid));
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".incDepth()"));
        if (action->getType() == ActionType::Reachability)
        {
            const auto& typeOfGraphToStates = pragmaRepeatFlatData_.getTypeOfGraphToStatesMap();
            const auto statesToClearIt = typeOfGraphToStates.find({action->getLeftSide(), action->getRightSide(), 0});
            if (statesToClearIt != typeOfGraphToStates.end())
            {
                for (const auto& cacheToClear : statesToClearIt->second)
                {
                    const auto stateName = graph_->getNode(cacheToClear)->getName();
                    const auto& stateCache = getStateCacheSafe(stateName);
                    const auto fullCacheName = mainCacheName_ + "." + stateCache->getCacheName();
                    tmpBlockInstruction->pushInstructionBack(
                        std::make_unique<CustomInstruction>(fullCacheName + stateCache->getResetInstruction()));
                }
            }
        }
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
    bool simpleApplyEdgeMode,
    const bool isCacheNeed)
{
    const std::string stateTo = toNode->getName();
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    if (!actions.empty() && common::isActionAssignmentToPlayer(actions.back()))
    {
        if (applyEdgeMode)
        {
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(
                std::string(CURRENT_STATE_WORD) + " = " + std::to_string(graph->getNodeId(stateTo))));
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
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("moves.emplace_back(mr)"));
            actions.pop_back();
        }
    }
    else
    {
        std::string stateFunctionArguments = "moves, mr";
        std::string stateName(STATE_WORD);

        if (applyEdgeMode)
        {
            stateFunctionArguments = "mr";
            stateName = std::string(APPLY_STATE_WORD);
        }

        if (!optNoCycleDetection_ && isCacheNeed)
        {
            stateFunctionArguments += "," + mainCacheName_;
        }

        const auto functionName =
            stateName + (preserveOriginalNames_ ? stateTo : std::to_string(graph->getNodeId(stateTo)));
        const auto functionCall = functionName + "(" + stateFunctionArguments + ")";
        functionCallCounter_[functionName]++;

        if (applyEdgeMode)
        {
            if (simpleApplyEdgeMode)
            {
                blockInstruction->pushInstructionBack(std::make_unique<ReturnInstruction>(functionCall));
            }
            else
            {
                std::unique_ptr<IfInstruction> ifInstruction =
                    std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(functionCall));
                ifInstruction->addInstruction(std::move(std::make_unique<ReturnInstruction>("true")));
                blockInstruction->pushInstructionBack(std::move(ifInstruction));
            }
        }
        else
        {
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(functionCall));
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
    bool skipFirstInstruction,
    const bool isCacheNeed)
{
    const std::string stateFrom = edge->fromName();
    const std::string stateTo = edge->toName();
    const auto& baseActions = graph->getEdge(stateFrom, stateTo, iid)->getActions();
    std::vector<std::shared_ptr<IAction>> actions(baseActions.begin(), baseActions.end());
    std::unique_ptr<BlockInstruction> blockInstruction = prepareBaseInstructions(
        graph, actions, edge->getRightNode(), applyEdgeMode, /*simpleApplyEdgeMode*/ false, isCacheNeed);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);
    auto skipFirstInstructionIter = actions.rend();
    if (skipFirstInstruction)
    {
        skipFirstInstructionIter--;
    }

    const auto& repeatNodes = pragmaRepeatFlatData_.getRepeatNodes();
    const auto edgeToStatesRequiringClear =
        graphOperatorManager_->getOperator<PragmaRepeatOperator>(graph)->getEdgeToStatesForWhichCacheShouldBeCleared(
            repeatNodes, graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgesWithActionTag());

    std::vector<std::shared_ptr<Node>> nodes = {edge->getLeftNode()};
    nodes.insert(nodes.end(), edge->getInnerNodes().begin(), edge->getInnerNodes().end());
    assert(nodes.size() == baseActions.size());
    nodes.push_back(edge->getRightNode());
    if (common::isActionAssignmentToPlayer(baseActions.back()))
    {
        // last action was erased in prepareBaseInstructions call, therefore we need to adjust set of nodes
        nodes.pop_back();
    }
    std::reverse(nodes.begin(), nodes.end());
    auto nodeIt = nodes.begin();
    std::shared_ptr<IAction> assignAnyAction = nullptr;
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
        else if (action->getType() == ActionType::Tag || action->getType() == ActionType::TagVariable)
        {
            const auto cachesToClear = edgeToStatesRequiringClear.find(edge);
            if (cachesToClear != edgeToStatesRequiringClear.end())
            {
                for (const auto& cacheToClear : cachesToClear->second)
                {
                    const auto stateName = graph->getNode(cacheToClear)->getName();
                    const auto& stateCache = getStateCacheSafe(stateName);
                    const auto fullCacheName = mainCacheName_ + "." + stateCache->getCacheName();
                    blockInstruction->pushInstructionFront(
                        std::make_unique<CustomInstruction>(fullCacheName + stateCache->getResetInstruction()));
                }
            }

            const auto tagValueStr = getTagValueString(action, edge);
            bool useArray = !maxMoveLen_ &&
                            graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();
            if (applyEdgeMode)
            {
                if (useArray)
                {
                    std::string pos = std::to_string(
                        graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->getTagPositionForNode(
                            edge->getLeftNode()->getName()));

                    std::unique_ptr<IfInstruction> ifInstruction;

                    ifInstruction = std::make_unique<IfInstruction>(
                        std::make_unique<ComparisonInstruction>("mr[" + pos + "] == " + tagValueStr));

                    ifInstruction->addInstruction(std::move(blockInstruction));
                    blockInstruction = std::make_unique<BlockInstruction>();
                    blockInstruction->pushInstructionBack(std::move(ifInstruction));
                }
                else
                {
                    blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(
                        mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "++"));
                    std::unique_ptr<IfInstruction> ifInstruction;

                    ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "static_cast<int>(mr.size()) > " + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) +
                        " && mr[" + mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "] == " + tagValueStr));

                    blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(
                        mainCacheName_ + "." + std::string(CURRENT_MR_ID_WORD) + "--"));
                    ifInstruction->addInstruction(std::move(blockInstruction));
                    blockInstruction = std::make_unique<BlockInstruction>();
                    blockInstruction->pushInstructionBack(std::move(ifInstruction));
                }
            }
            else
            {
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
                    const auto pushTag = "mr.emplace_back(" + tagValueStr + ")";
                    blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(pushTag));
                    blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("mr.pop_back()"));
                }
            }
        }
        else if (action->getType() == ActionType::Reachability)
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
        else if (action->getType() == ActionType::AssignmentAny)
        {
            if (assignAnyAction != nullptr)
            {
                throw std::runtime_error("[Compiler] Multiple nodes of type AssignmentAny are not supported.");
            }
            assignAnyAction = action;
        }
    }

    if (assignAnyAction)
    {
        const auto tmpVarName = getTemporaryVariableName(temporaryVariableCnt, edgeId);
        return wrapIntoLoopIfNeeded(assignAnyAction, std::move(blockInstruction), tmpVarName);
    }
    return blockInstruction;
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
    BoolFunctionType patternId,
    bool isCacheNeed)
{
    const std::string& stateFrom = edge->fromName();
    const std::string& stateTo = edge->toName();
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    std::string functionArguments;
    if (!optNoCycleDetection_ && isCacheNeed)
    {
        functionArguments = mainCacheName_;
    }

    const auto functionName = prefix + (preserveOriginalNames_ ? stateTo : std::to_string(graph_->getNodeId(stateTo)));
    functionCallCounter_[functionName]++;
    std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
        std::make_unique<ComparisonInstruction>(functionName + "(" + functionArguments + ")"));

    if (patternId == BoolFunctionType::Default)
    {
        std::vector<std::shared_ptr<IAction>> assignmentActions;

        for (const auto& action : actions)
        {
            if (action->getType() == ActionType::Assignment || action->getType() == ActionType::AssignmentAny)
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
    bool isApplyAnyMove,
    bool addReturn,
    bool skipFirstInstruction,
    const bool isCacheNeed)
{
    const std::string& stateFrom = edge->fromName();
    const std::string& stateTo = edge->toName();
    std::string prefix = std::string(IS_LEGAL_WORD);

    if (isApplyAnyMove)
    {
        prefix = std::string(APPLY_ANY_WORD);
    }

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
    std::unique_ptr<BlockInstruction> blockInstruction = prepareBaseInstructions(
        graph,
        actions,
        from,
        to,
        edgeIdx,
        edge,
        iid,
        prefix,
        !isApplyAnyMove ? BoolFunctionType::Default : BoolFunctionType::ApplyAny,
        isCacheNeed);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);
    std::shared_ptr<IAction> assignAnyAction = nullptr;
    auto skipFirstInstructionIter = actions.rend();
    if (skipFirstInstruction)
    {
        skipFirstInstructionIter--;
    }

    for (auto action_iterator = actions.rbegin(); action_iterator != actions.rend(); action_iterator++)
    {
        auto action = *action_iterator;
        if (action_iterator == skipFirstInstructionIter)
        {
            break;
        }
        if (action->getType() == ActionType::Assignment)
        {
            if (isApplyAnyMove && common::isActionAssignmentToPlayer(action))
            {
                blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>("return true"));
                blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>(
                    std::string(CURRENT_STATE_WORD) + " = " + std::to_string(graph_->getNodeId(stateTo))));
            }
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
                ifInstruction->addInstruction(std::make_unique<CustomInstruction>("return false"));
            }
            blockInstruction = std::make_unique<BlockInstruction>();
            blockInstruction->pushInstructionBack(std::move(ifInstruction));
        }
        else if (action->getType() == ActionType::Reachability)
        {
            std::unique_ptr<CustomInstruction> returnInstruction = nullptr;
            if (addReturn)
            {
                returnInstruction = std::make_unique<CustomInstruction>("return false");
            }
            blockInstruction = addActionPattern(
                action, graph, stateFrom, stateTo, iid, std::move(blockInstruction), std::move(returnInstruction));
        }
        else if (action->getType() == ActionType::AssignmentAny)
        {
            if (assignAnyAction != nullptr)
            {
                throw std::runtime_error("[Compiler] Multiple nodes of type AssignmentAny are not supported.");
            }
            assignAnyAction = action;
        }
    }

    if (assignAnyAction)
    {
        const auto tmpVarName = getTemporaryVariableName(temporaryVariableCnt, edgeId);
        return wrapIntoLoopIfNeeded(assignAnyAction, std::move(blockInstruction), tmpVarName);
    }
    return blockInstruction;
}

void Compiler::generateGetFromStateForEdge(const std::shared_ptr<Graph>& graph)
{
    auto function = std::make_unique<Function>("getFromStateForEdge", std::string(CUSTOM_TYPE_WORD), "", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", std::string(CUSTOM_TYPE_WORD)));

    auto sw = std::make_unique<SwitchInstruction>("val");

    const auto& edges = graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgeNames();

    for (const auto& [stateFrom, stateTo, edgeId] : edges)
    {
        const auto& actionBack = graph->getEdge(stateFrom, stateTo, edgeId)->getActions().back();
        const auto& actionFront = graph->getEdge(stateFrom, stateTo, edgeId)->getActions().front();
        std::string id = std::to_string(graph->getNodeId(stateTo));

        if (common::isActionAssignmentToPlayer(actionBack))
        {
            sw->addCaseInstruction(
                graph->getEdgeId(stateFrom, stateTo, edgeId), std::move(std::make_unique<ReturnInstruction>(id)));
        }
        else if (
            actionBack->getType() == ActionType::Tag || actionBack->getType() == ActionType::TagVariable ||
            stateFrom == common::BEGIN_WORD)
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
    function->addInstruction(std::make_unique<ReturnInstruction>("ss.str()"));
    program_.addFunction(std::move(function));
}

void Compiler::generateRunStateFunction(const std::shared_ptr<Graph>& graph, bool applyMode)
{
    const auto functionName = (applyMode ? "runApplyState" : "runState");
    const auto prefix = (applyMode ? std::string(APPLY_STATE_WORD) : std::string(STATE_WORD));

    auto function = std::make_unique<Function>(functionName, "void", "", false);
    function->addArgument(std::make_unique<VariableDeclarationInstruction>("val", std::string(CUSTOM_TYPE_WORD)));
    function->setAttributes("inline");

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
    std::string functionArguments = (applyMode ? "mr" : "moves, mr");
    if (!optNoCycleDetection_ &&
        checkIsCacheNeed((applyMode ? std::string(APPLY_STATE_WORD) : std::string(STATE_WORD)), graph))
    {
        functionArguments += "," + mainCacheName_;
    }

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (const auto& [edge, iid] :
         graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgesWithActionChangePlayerButNotKeeper())
    {
        const auto stateFunctionName =
            prefix + (preserveOriginalNames_ ? edge->toName() : std::to_string(graph->getNodeId(edge->toName())));
        functionCallCounter_[stateFunctionName]++;
        auto block = std::make_unique<BlockInstruction>();
        block->pushInstructionBack(
            std::make_unique<CustomInstruction>(stateFunctionName + "(" + functionArguments + ")"));
        block->pushInstructionBack(std::make_unique<ReturnInstruction>());

        sw->addCaseInstruction(graph->getNodeId(edge->toName()), std::move(block));
    }

    const auto stateBeginName =
        prefix + (preserveOriginalNames_ ? std::string(common::BEGIN_WORD)
                                         : std::to_string(graph->getNodeId(common::BEGIN_WORD)));
    functionCallCounter_[stateBeginName]++;
    auto block = std::make_unique<BlockInstruction>();
    block->pushInstructionBack(std::make_unique<CustomInstruction>(stateBeginName + "(" + functionArguments + ")"));
    block->pushInstructionBack(std::make_unique<ReturnInstruction>());

    sw->addCaseInstruction(graph->getNodeId(common::BEGIN_WORD), std::move(block));

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateSpecialFunctions(const std::shared_ptr<Graph>& graph)
{
    generateRunStateFunction(graph, true);
    generateRunStateFunction(graph);
    generateGetStateDescription();
    //tutaj
    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", "", true, true);
    isTerminal->addInstruction(std::make_unique<ReturnInstruction>(
        std::string(CURRENT_STATE_WORD) + " == " + std::to_string(graph->getNodeId(common::END_WORD))));

    auto getPlayerScore = std::make_unique<Function>("getPlayerScore", "Score", "", true, true);
    getPlayerScore->addArgument(std::make_unique<VariableDeclarationInstruction>(
        std::string(common::PLAYER_WORD), std::string(common::PLAYER_TYPE_WORD)));
    getPlayerScore->addInstruction(
        std::make_unique<ReturnInstruction>("goals[" + std::string(common::PLAYER_WORD) + "- 1]"));

    auto getCurrentPlayer =
        std::make_unique<Function>("getCurrentPlayer", std::string(common::PLAYER_OR_SYSTEM_TYPE_WORD), "", true, true);
    getCurrentPlayer->addInstruction(std::make_unique<ReturnInstruction>(std::string(common::PLAYER_WORD)));

    auto getAllMovesFunction = std::make_unique<Function>("getAllMoves", "void", "", true);
    getAllMovesFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("moves", "std::vector<Move>&"));
    getAllMovesFunction->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, mainCacheType_ + "&"));
    std::string clearingCaches = mainCacheName_ + ".reset();\n";
    if (verification_)
    {
        clearingCaches += "verificationCache.clear();\n";
    }
    bool useArray =
        !maxMoveLen_ && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();

    if (useArray)
    {
        getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
            clearingCaches + "moves.clear();\nMove mr;\nmr.mr.fill(-1);\nrunState(currentState, moves,mr.mr," +
            mainCacheName_ + ")"));
    }
    else
    {
        getAllMovesFunction->addInstruction(std::make_unique<CustomInstruction>(
            clearingCaches + "moves.clear();\nMove mr;\nrunState(currentState, moves,mr.mr," + mainCacheName_ + ")"));
    }

    auto applyMoveFunction = std::make_unique<Function>("applyMove", "void", "", true);
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("m", "const Move&"));
    applyMoveFunction->addArgument(std::make_unique<VariableDeclarationInstruction>("rgCache", "RgCache&"));
    applyMoveFunction->addInstruction(std::make_unique<CustomInstruction>(mainCacheName_ + ".reset()"));
    applyMoveFunction->addInstruction(
        std::make_unique<CustomInstruction>("runApplyState(currentState, m.mr, rgCache)"));

    program_.addFunction(std::move(isTerminal));
    program_.addFunction(std::move(getPlayerScore));
    program_.addFunction(std::move(getCurrentPlayer));
    program_.addFunction(std::move(getAllMovesFunction));
    program_.addFunction(std::move(applyMoveFunction));
}

void Compiler::generateApplyAnyMove()
{
    generatePatternFunctions(applyAnyMoveGraphs_);

    auto function = std::make_unique<Function>("applyAnyMove", "void", "", true);
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>(mainCacheName_, "[[maybe_unused]]" + mainCacheType_ + "&"));

    auto sw = std::make_unique<SwitchInstruction>(std::string(CURRENT_STATE_WORD));
    bool cacheNeeded = false;
    std::string cacheName = "applyAnyCache";
    for (const auto& [nodeName, findalNodes, graph] : applyAnyMoveGraphs_)
    {
        auto block = std::make_unique<BlockInstruction>();

        std::string functionArguments;
        if (!optNoCycleDetection_)
        {
            bool skipStateCache = areAllNodesInApplyAnyGraphUnique_.count(nodeName);
            if (checkIsCacheNeed(std::string(APPLY_ANY_WORD), graph))
            {
                functionArguments += mainCacheName_;
            }
            cacheNeeded |= !skipStateCache;
        }

        std::string functionName = std::to_string(graph_->getNodeId(nodeName)) + "_" +
                                   std::to_string(graph_->getNodeId(nodeName)) + "_" +
                                   std::to_string(graph_->getNodeId(nodeName));
        if (preserveOriginalNames_)
        {
            functionName = nodeName + "_" + nodeName + "_" + nodeName;
        }
        functionName = std::string(APPLY_ANY_WORD) + functionName;
        functionCallCounter_[functionName]++;
        std::unique_ptr<CustomInstruction> custom =
            std::make_unique<CustomInstruction>(functionName + "(" + functionArguments + ")");

        block->pushInstructionBack(std::move(custom));

        if (cacheNeeded)
        {
            block->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".reset()"));
        }
        block->pushInstructionBack(std::make_unique<CustomInstruction>("break"));

        sw->addCaseInstruction(graph_->getNodeId(nodeName), std::move(block));
    }

    if (cacheNeeded)
    {
        auto tmpBlockInstruction = std::make_unique<BlockInstruction>();
        tmpBlockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>(mainCacheName_ + ".reset()"));
        function->addInstruction(std::move(tmpBlockInstruction));
    }

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generatePatternFunctions(
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, BoolFunctionType patternId)
{
    for (const auto& [from, to, graph] : patterns)
    {
        generateBoolStateFunctions(from, to, graph, patternId, areAllNodesInPatternGraphUnique_.count({from, to}));
    }
}

void Compiler::generatePatternFunctions(
    std::vector<std::tuple<std::string, std::set<int>, std::shared_ptr<Graph>>> patterns, BoolFunctionType patternId)
{
    for (const auto& [from, to, graph] : patterns)
    {
        generateBoolStateFunctions(from, from, graph, patternId, areAllNodesInApplyAnyGraphUnique_.count(from), to);
    }
}

void Compiler::generatePatternReachabilityFunctions()
{
    calculatePatternsWithCache();
    generatePatternFunctions(patternReachabilityGraphs_);
}

void Compiler::generateFunctions()
{
    generatePatternReachabilityFunctions();
    generateVoidStateFunctions(graph_);
    generateVoidStateFunctions(graph_, true);
    generateApplyAnyMove();
    generateSpecialFunctions(graph_);

    if (optGccInline_ == InlineMode::SingleCall)
    {
        for (const auto& function : program_.getFunctions())
        {
            if (!function->isPublic() && functionCallCounter_[function->getName()] <= 1)
            {
                function->setAttributes("inline");
            }
        }
    }
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
    const std::shared_ptr<IAction>& actionAssignAny,
    std::unique_ptr<BlockInstruction> blockInstruction,
    const std::string& tmpVariableName) const
{
    // Optimization: not create loops when only one value will be accepted
    const auto variableName = actionAssignAny->getLeftSide();
    const auto ifInstruction = dynamic_cast<IfInstruction*>(blockInstruction->frontInstruction().get());
    if (ifInstruction != nullptr && ifInstruction->getCondition()->getType() == ComparisonType::Eq)
    {
        const auto [lhs, rhs] = ifInstruction->getCondition()->getSubexpressions();
        if (lhs == variableName)
        {
            auto instructionsInsideIf = ifInstruction->extractInstructions();
            blockInstruction->popInstructionFront();
            for (auto& insideInstruction : std::ranges::reverse_view(instructionsInsideIf))
            {
                blockInstruction->pushInstructionFront(std::move(insideInstruction));
            }
            return blockInstruction;
        }
        if (rhs == variableName)
        {
            auto instructionsInsideIf = ifInstruction->extractInstructions();
            blockInstruction->popInstructionFront();
            for (auto& insideInstruction : std::ranges::reverse_view(instructionsInsideIf))
            {
                blockInstruction->pushInstructionFront(std::move(insideInstruction));
            }
            return blockInstruction;
        }
    }
    const LoopFactory loopFactory(parser_, valueAssigner_);
    auto loopInstruction = loopFactory.createLoopInstruction(*actionAssignAny);
    if (dynamic_cast<RangeLoopInstruction*>(loopInstruction.get()))
    {
        blockInstruction->pushInstructionFront(
            std::make_unique<AssignmentInstruction>(variableName, variableName + "It"));
    }
    loopInstruction->addInstruction(std::move(blockInstruction));
    auto result = std::make_unique<BlockInstruction>();
    result->pushInstructionBack(std::make_unique<AssignmentInstruction>(tmpVariableName, variableName, "const auto"));
    result->pushInstructionBack(std::move(loopInstruction));
    result->pushInstructionBack(std::make_unique<AssignmentInstruction>(variableName, tmpVariableName));
    return result;
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
    const auto tagName = action->getLeftSide();
    if (action->getType() == ActionType::Tag)
    {
        return std::to_string(valueAssigner_.getBaseValueForTag(tagName));
    }
    if (action->getType() == ActionType::TagVariable)
    {
        return std::to_string(valueAssigner_.getBaseValueForTag(tagName)) + " + " + tagName;
    }
    throw std::invalid_argument("[Compiler] Cannot extract tag from action: " + action->toString());
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

bool Compiler::isStateDisjoint(const std::string& state)
{
    return pragmaDisjointEnabled_ &&
           graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph_)->isDisjoint(state);
}

void Compiler::handleBoolDisjoint(
    const std::string& from,
    const std::string& to,
    std::unique_ptr<Function>& function,
    std::shared_ptr<Graph> graph,
    const std::string& state,
    bool isApplyAnyMove,
    const bool isCacheNeed)
{
    auto vectorOfNodeNames = graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph_)->getNodeNames(state);
    bool disjointExhaustive = graphOperatorManager_->getOperator<PragmaDisjointOperator>(graph_)->isExhaustive(state);
    int cnt = 0;
    std::set<std::string> visited;

    visited.insert(vectorOfNodeNames.begin(), vectorOfNodeNames.end());
    int numberOfDisjointEdgesInPatternGraph = 0;
    int edgeIdx = 0;

    for (auto [outgoingEdge, iid] : graph->getOutgoingEdgesFrom(state))
    {
        if (!visited.count(outgoingEdge->getRightNode()->getName()))
        {
            function->addInstruction(generateBoolEdgeInstruction(
                from,
                to,
                graph,
                outgoingEdge,
                iid,
                edgeIdx++,
                /*isApplyAnyMove*/ isApplyAnyMove,
                /*addReturn*/ false,
                /*skipFirstInstruction*/ false,
                isCacheNeed));
        }
        else
        {
            numberOfDisjointEdgesInPatternGraph++;
        }
    }
    if (numberOfDisjointEdgesInPatternGraph != vectorOfNodeNames.size())
    {
        disjointExhaustive = false;
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
                function->addInstruction(generateBoolEdgeInstruction(
                    from,
                    to,
                    graph,
                    outgoingEdge,
                    iid,
                    edgeIdx++,
                    /*isApplyAnyMove=*/isApplyAnyMove,
                    /*addReturn=*/true,
                    disjointExhaustive && ++cnt == numberOfDisjointEdgesInPatternGraph,
                    isCacheNeed));
            }
        }
    }
}

bool Compiler::checkIsCacheNeed(const std::string& functionName, const std::shared_ptr<Graph>& graph)
{
    bool allUnique =
        graphOperatorManager_->getOperator<PragmaUniqueOperator>(graph)->areAllNodesWithPragmaUnique(pragmaUniqueData_);
    if (functionName.starts_with(APPLY_STATE_WORD))
    {
        // Commented because of oware bug
        // return !(allUnique && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition());
        return true;
    }
    else if (allUnique && patternsWithCache_.empty())
    {
        return false;
    }

    return true;
}

bool Compiler::checkIsCacheNeedForPattern(const std::string& from, const std::string to) const
{
    return patternsWithCache_.count({from, to});
}

void Compiler::calculatePatternsWithCache()
{
    for (const auto& [from, to, graph] : patternReachabilityGraphs_)
    {
        if (!graphOperatorManager_->getOperator<PragmaUniqueOperator>(graph)->areAllNodesWithPragmaUnique(
                pragmaUniqueData_))
        {
            patternsWithCache_.insert({from, to});
        }
    }
    std::map<std::pair<std::string, std::string>, int> patternNameToId;
    for (const auto& [from, to, graph] : patternReachabilityGraphs_)
    {
        patternNameToId[{from, to}] = patternNameToId.size();
    }

    std::set<int> patternsWithCache;
    for (const auto& [from, to] : patternsWithCache_)
    {
        patternsWithCache.insert(patternNameToId[{from, to}]);
    }

    std::map<int, std::set<int>> patternsDependency;
    for (const auto& [from, to, graph] : patternReachabilityGraphs_)
    {
        fillPaternsDependency(graph, patternsDependency, patternNameToId, patternNameToId.at({from, to}));
    }

    std::set<int> patternsWithCacheResults;
    for (int patternId : patternsWithCache)
    {
        visitAdjacentPatterns(patternId, patternsDependency, patternsWithCacheResults);
    }
    patternsWithCache_.clear();
    for (const auto& pairNodeNameAndId : patternNameToId)
    {
        if (patternsWithCacheResults.count(pairNodeNameAndId.second))
        {
            patternsWithCache_.insert(pairNodeNameAndId.first);
        }
    }
}

bool Compiler::arePatternAndMainGraphUnique()
{
    return patternsWithCache_.empty() &&
           graphOperatorManager_->getOperator<PragmaUniqueOperator>(graph_)->areAllNodesWithPragmaUnique(
               pragmaUniqueData_);
}