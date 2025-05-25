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
constexpr const int SMALL_VECTOR_MOVE_SIZE = 12;
const std::string UNUSED_TAG_VALUE = "-1";

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

    pragmaRepeatFlatData_.parse(parser_);

    pragmaRepeatFlatData_.initializeDataForGraphs(patternReachabilityGraphs_, graph_, 0);
    pragmaRepeatFlatData_.initializeDataForGraphs(applyAnyMoveGraphs_, graph_, 1);
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

    initializePatternGraphs(patternReachabilityGraphs_);
    initializePatternGraphs(applyAnyMoveGraphs_);
}

void Compiler::initializePatternGraphs(
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>>& patterns)
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
    printer.printMainCache(containerChooser_.getAdditionalData(stateToCache_));
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

    const std::string initialState = std::to_string(graph->getNodeId("begin"));
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

        if (!optNoCycleDetection_)
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

        program_.addFunction(std::move(function));
    }
}

std::unique_ptr<BlockInstruction> Compiler::getAssignments(
    const std::vector<std::shared_ptr<IAction>>& actions,
    const std::vector<std::string>& tags,
    std::vector<int>& minValues) const
{
    int curentPos = 1;
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();
    std::map<std::string, std::string> tagToValue;
    for (auto tag : tags)
    {
        auto tagVar = getTagVar(tag);
        if (tagVar)
        {
            tagToValue[*tagVar] = "mr[currentMrId - " + std::to_string(minValues.size() - curentPos + 1) + "] - " +
                                  std::to_string(minValues[curentPos - 1]);
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
        std::string tag = pairFullTagAndChild.first;
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
                bool useArray =
                    !maxMoveLen_ &&
                    graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();

                std::unique_ptr<IfInstruction> ifInstruction;
                if (useArray)
                {
                    ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                        "static_cast<int>(mr.size()) > currentMrId && mr[currentMrId]", "-1", ComparisonType::Neq));
                }
                else
                {
                    ifInstruction = std::make_unique<IfInstruction>(
                        std::make_unique<ComparisonInstruction>("static_cast<int>(mr.size()) > currentMrId"));
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
    const bool hasAnyEmptyTagSequence)
{
    static int nameCnt = 0;
    std::string functionName = "switch_" + std::to_string(nameCnt++);
    std::unique_ptr<Function> function = std::make_unique<Function>(functionName, "bool");
    function->addArgument(
        std::make_unique<VariableDeclarationInstruction>("mr", "[[maybe_unused]]const move_representation&"));

    if (!optNoCycleDetection_)
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
        auto switchBody =
            makeSwitchForTags(listOfActionsToTags, tags, 1, isExhaustive, hasAnyEmptyTagSequence, minValues);
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
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size()) > currentMrId && mr[currentMrId]", "-1", ComparisonType::Neq));
            }
            else
            {
                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size())", "currentMrId", ComparisonType::Gr));
            }
            ifInstruction->addInstruction(std::move(switchBody));
            blockAction = std::move(ifInstruction);
        }

        blockInstruction->pushInstructionFront(std::move(blockAction));
    }

    if (!listOfActionsToPlayerChangeAndEndNode.first.empty())
    {
        std::vector<std::shared_ptr<IAction>> actonsToPlayerChangeAndEndNode =
            listOfActionsToPlayerChangeAndEndNode.first;
        std::vector<int> minValuesEmpty;
        std::unique_ptr<BlockInstruction> blockInstructionTmp =
            getAssignments(actonsToPlayerChangeAndEndNode, {}, minValuesEmpty);

        blockInstructionTmp->pushInstructionBack(prepareBaseInstructions(
            unoptimizedGraph_, actonsToPlayerChangeAndEndNode, listOfActionsToPlayerChangeAndEndNode.second, true));

        bool useArray =
            !maxMoveLen_ && graphOperatorManager_->getOperator<GetTagIndexOperator>(graph_)->allTagsInSamePosition();

        std::unique_ptr<IfInstruction> ifInstruction;
        if (useArray)
        {
            ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                "static_cast<int>(mr.size()) > currentMrId && mr[currentMrId]", "-1", ComparisonType::Neq));
        }
        else
        {
            ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                "static_cast<int>(mr.size())", "currentMrId", ComparisonType::Neq));
        }

        ifInstruction->addInstruction(std::make_unique<ReturnInstruction>("false"));
        blockInstruction->pushInstructionBack(std::move(ifInstruction));
        blockInstruction->pushInstructionBack(std::move(blockInstructionTmp));
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
    const auto functionCall = functionName + "(mr, " + mainCacheName_ + ")";
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
    bool skipStateCache)
{
    std::string name(patternIdToPrefixName.at(patternId));
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
    std::string prefix = "is_legal_";
    std::string functionName = prefix + fromNode + "_" + toNode + "_" + fromNode;

    if (preserveOriginalNames_)
    {
        functionName = prefix + action->getLeftSide() + "_" + action->getRightSide() + "_" + action->getLeftSide();
    }

    std::string functionArguments;
    std::unique_ptr<BlockInstruction> tmpBlockInstruction = std::make_unique<BlockInstruction>();

    bool skipStateCache = areAllNodesInPatternGraphUnique_.count({action->getLeftSide(), action->getRightSide()});
    functionArguments += mainCacheName_;

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
            blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("moves.emplace_back(mr)"));
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

    const auto& repeatNodes = pragmaRepeatFlatData_.getRepeatNodes();
    const auto edgeToStatesRequiringClear =
        graphOperatorManager_->getOperator<PragmaRepeatOperator>(graph)->getEdgeToStatesForWhichCacheShouldBeCleared(
            repeatNodes, graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgesWithActionTag());

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
            if (applyEdgeMode)
            {
                blockInstruction->pushInstructionFront(std::make_unique<CustomInstruction>("currentMrId++"));
                std::unique_ptr<IfInstruction> ifInstruction;

                ifInstruction = std::make_unique<IfInstruction>(std::make_unique<ComparisonInstruction>(
                    "static_cast<int>(mr.size()) > currentMrId && mr[currentMrId] == " + tagValueStr));

                blockInstruction->pushInstructionBack(std::make_unique<CustomInstruction>("currentMrId--"));
                ifInstruction->addInstruction(std::move(blockInstruction));
                blockInstruction = std::make_unique<BlockInstruction>();
                blockInstruction->pushInstructionBack(std::move(ifInstruction));
            }
            else
            {
                bool useArray =
                    !maxMoveLen_ &&
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
    BoolFunctionType patternId)
{
    const std::string& stateFrom = edge->fromName();
    const std::string& stateTo = edge->toName();
    std::unique_ptr<BlockInstruction> blockInstruction = std::make_unique<BlockInstruction>();

    std::string functionArguments;
    if (!optNoCycleDetection_)
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
    std::unique_ptr<BlockInstruction> blockInstruction = prepareBaseInstructions(
        graph,
        actions,
        from,
        to,
        edgeIdx,
        edge,
        iid,
        prefix,
        functionType.empty() ? BoolFunctionType::Default : BoolFunctionType::ApplyAny);
    int temporaryVariableCnt = 0;
    int edgeId = graph->getEdgeId(stateFrom, stateTo, iid);
    std::shared_ptr<IAction> assignAnyAction = nullptr;
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
        }
        else if (action->getType() == ActionType::Reachability)
        {
            blockInstruction = addActionPattern(action, graph, stateFrom, stateTo, iid, std::move(blockInstruction));
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
        else if (
            actionBack->getType() == ActionType::Tag || actionBack->getType() == ActionType::TagVariable ||
            stateFrom == "begin")
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
    const auto functionName = (applyMode ? "runApplyState" : "runState");
    const auto prefix = (applyMode ? "apply_state_" : "state_");

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
    std::string functionArguments = (applyMode ? "mr" : "moves, mr");
    if (!optNoCycleDetection_)
    {
        functionArguments += "," + mainCacheName_;
    }

    auto sw = std::make_unique<SwitchInstruction>("val");

    for (const auto& [edge, iid] :
         graphOperatorManager_->getOperator<GetEdgeOperator>(graph)->getEdgesWithActionChangePlayer())
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

    const auto stateBeginName = prefix + (preserveOriginalNames_ ? "begin" : std::to_string(graph->getNodeId("begin")));
    functionCallCounter_[stateBeginName]++;
    auto block = std::make_unique<BlockInstruction>();
    block->pushInstructionBack(std::make_unique<CustomInstruction>(stateBeginName + "(" + functionArguments + ")"));
    block->pushInstructionBack(std::make_unique<ReturnInstruction>());

    sw->addCaseInstruction(graph->getNodeId("begin"), std::move(block));

    function->addInstruction(std::move(sw));
    program_.addFunction(std::move(function));
}

void Compiler::generateSpecialFunctions(const std::shared_ptr<Graph>& graph)
{
    generateRunStateFunction(graph, true);
    generateRunStateFunction(graph);
    generateGetStateDescription();

    auto isTerminal = std::make_unique<Function>("isTerminal", "bool", "", true);
    isTerminal->addInstruction(
        std::make_unique<ReturnInstruction>("currentState == " + std::to_string(graph->getNodeId("end"))));

    auto getPlayerScore = std::make_unique<Function>("getPlayerScore", "Score", "", true);
    getPlayerScore->addArgument(std::make_unique<VariableDeclarationInstruction>("player", "Player"));
    getPlayerScore->addInstruction(std::make_unique<ReturnInstruction>("goals[player - 1]"));

    auto getCurrentPlayer = std::make_unique<Function>("getCurrentPlayer", "PlayerOrSystem", "", true);
    getCurrentPlayer->addInstruction(std::make_unique<ReturnInstruction>("player"));

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
    applyMoveFunction->addInstruction(std::make_unique<AssignmentInstruction>("currentMrId", "0"));
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
    generatePatternFunctions(applyAnyMoveGraphs_, BoolFunctionType::ApplyAny);

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
            functionName = "is_legal_apply_any_" + functionName;
            functionCallCounter_[functionName]++;
            std::unique_ptr<IfInstruction> ifInstruction = std::make_unique<IfInstruction>(
                std::make_unique<ComparisonInstruction>(functionName + "(" + functionArguments + ")"));
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
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, BoolFunctionType patternId)
{
    for (const auto& [from, to, graph] : patterns)
    {
        bool skipCache = false;
        if (patternId == BoolFunctionType::ApplyAny)
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

void Compiler::generateFunctions()
{
    generateVoidStateFunctions(graph_);
    generateVoidStateFunctions(graph_, true);
    generatePatternReachabilityFunctions();
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
