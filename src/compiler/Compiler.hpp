#pragma once

#include <compiler/ValueAssigner.hpp>
#include <compiler/cacheContainers/ContainerChooser.hpp>
#include <compiler/graphOperations/GraphOperatorManager.hpp>
#include <graph/ActionFactory.hpp>
#include <graph/Edge.hpp>
#include <graph/Graph.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

struct Options
{
    bool simplePathCompression_;
    bool noCycleDetection_;
    bool printOriginalNames_;
    bool preserveOriginalNames_;
    bool verification_;
    bool pragmaDisjointEnabled_;
    int game_;
};

class Compiler
{
    std::string hs_;
    std::string hs2_;

public:
    Compiler(const Parser &parser, const Options &options);
    void compile();
    void generateSourceCode(std::ofstream &headerFile, std::ofstream &sourceFile);

private:
    void initializeGraph();
    void generateTypes();
    void generateConstants();
    void generateFunctions();
    void generateVariables(const std::shared_ptr<Graph> &graph);
    void generateVoidStateFunctions(const std::shared_ptr<Graph> &graph, bool applyMode = false);
    void generateBoolStateFunctions(
        const std::string &from,
        const std::string &to,
        const std::shared_ptr<Graph> &graph,
        int patternId = 0,
        bool skipStateCache = false);
    std::unique_ptr<BlockInstruction> addActionPattern(
        const std::shared_ptr<IAction> &action,
        const std::shared_ptr<Graph> &graph,
        const std::string &stateFrom,
        const std::string &stateTo,
        int iid,
        std::unique_ptr<BlockInstruction> blockInstruction,
        std::unique_ptr<CustomInstruction> returnInstruction = nullptr);
    std::unique_ptr<BlockInstruction> getAssignments(
        const std::vector<int> &edges,
        const std::shared_ptr<Graph> &graph,
        const std::string &currentTagFromVector = "",
        const std::string &fullTagName = "",
        const std::string &minVal = "",
        int commonPrefixSize = 0) const;
    std::unique_ptr<BlockInstruction> makeSwitchForTags(
        const std::shared_ptr<SimpleApplySwitchTreeNode> &listOfActionsToTags,
        int depth,
        int minVal = 0,
        const std::string &fullTagName = "",
        bool isExhaustive = false);

    std::vector<std::shared_ptr<IAction>> getAssignmentsList(
        const std::vector<int> &edges, const std::shared_ptr<Graph> &graph, int commonPrefixSize = 0) const;
    //int getCommonPrefixSize(const std::vector<TagAndListOfEdges> &tagsAndEdges) const;
    std::unique_ptr<BlockInstruction> generateVoidEdgeInstruction(
        const std::shared_ptr<Graph> &graph,
        const std::shared_ptr<Edge> &edge,
        int iid,
        bool applyEdgeMode = false,
        bool addReturn = false,
        bool skipFirstInstruction = false);
    std::unique_ptr<BlockInstruction> generateVoidEdgeInstruction(
        const std::shared_ptr<SimpleApplySwitchTreeNode> &listOfActionsToTags,
        const std::vector<int> &listOfActionsToPlayerChange,
        bool isExhaustive);
    std::unique_ptr<BlockInstruction> prepareBaseInstructions(
        const std::shared_ptr<Graph> &graph,
        std::vector<std::shared_ptr<IAction>> &actions,
        const std::shared_ptr<Edge> &edge,
        int iid,
        bool applyEdgeMode,
        bool simpleApplyEdgeMode = false);
    std::unique_ptr<BlockInstruction> generateBoolEdgeInstruction(
        const std::string &from,
        const std::string &to,
        const std::shared_ptr<Graph> &graph,
        const std::shared_ptr<Edge> &edge,
        int iid,
        int edgeIdx);
    std::unique_ptr<BlockInstruction> prepareBaseInstructions(
        const std::shared_ptr<Graph> &graph,
        const std::vector<std::shared_ptr<IAction>> &actions,
        const std::string &patternFrom,
        const std::string &patternTo,
        int edgeIdx,
        const std::shared_ptr<Edge> &edge,
        int iid,
        const std::string &cacheName,
        const std::string &prefix,
        int patternId = 0);
    void generateSpecialFunctions(const std::shared_ptr<Graph> &graph);
    //void generateRunApplyEdgeFunction(const std::shared_ptr<Graph> &graph);
    void generateRunStateFunction(const std::shared_ptr<Graph> &graph, bool applyMode = false);
    void generateGetFromStateForEdge(const std::shared_ptr<Graph> &graph);
    void generateVoidStateOptimizedFunction(
        const std::string &state,
        const std::unique_ptr<Function> &function,
        const std::shared_ptr<Graph> &graph,
        bool applyMode = false);
    void generatePatternFunctions(
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, int patternId = 0);
    void generatePatternReachabilityFunctions();
    void generatePatternAnyFunctions();
    void generateApplyAnyMove();
    void initializePatternGraphs(
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> &patterns, int patternId = 0);
    template<typename T>
    void restoreAssignments(
        const std::unique_ptr<T> &function, std::vector<std::shared_ptr<IAction>> assignments, int edgeId);
    std::string getStateIntId(std::string name);
    std::string getMoveRepresentation();
    std::shared_ptr<IType> generateType(const nlohmann::json &t);
    std::shared_ptr<IType> generateFunctionType(const nlohmann::json &t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json &value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json &value);
    std::unique_ptr<IInstruction> debugInstruction(std::string functionName);
    std::string getTemporaryVariableName(int idx, int edgeId);
    int getNumberOfPlayers();
    int getDomain(const std::string &s);
    void initializePragmaVerticesSet(const std::string &pragmaName, std::set<std::string> &data);
    void initializePragmaDisjoint();
    void initializePragmas();
    void initializePragmaRepeat();
    void initializePragmaUnique();
    void initializePragmaSimpleApply();
    std::string getTypeForVariable(const std::string &variableName);
    std::string getTagValueString(const std::shared_ptr<IAction> &action, const std::shared_ptr<Edge> &edge);
    std::string getVariableValueFromTagString(const std::shared_ptr<Edge> &edge) const;

    bool nodeInThisEdge(const std::shared_ptr<Edge> &edge, const std::string &nodeName) const;

    const Parser &parser_;
    ValueAssigner valueAssigner_;
    std::shared_ptr<Graph> graph_;
    std::shared_ptr<Graph> unoptimizedGraph_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternReachabilityGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternAnyGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> applyAnyMoveGraphs_;
    std::map<std::tuple<std::string, std::string, int>, std::set<std::string>> variablesInPatternGraphs_;
    Program program_;
    const std::string temporaryVariableNamePrefix_;
    const bool printOriginalNames_;
    const bool preserveOriginalNames_;
    const bool pragmaDisjointEnabled_;
    const bool verification_;
    const bool optConditionsSimplePathCompression_;
    const bool optNoCycleDetection_;
    int game_;
    const std::string patternIdToPrefixName[3] = {"", "any_", "any2_"};
    const std::string mainCacheName_;
    const std::string mainCacheType_;
    ContainerChooser containerChooser_;
    std::shared_ptr<GraphOperatorManager> graphOperatorManager_;
    std::set<std::string> pragmaUniqueData_;
    std::set<std::string> pragmaSimpleApplyData_;
    std::map<std::string, std::vector<std::string>> pragmaRepeatData_;
    std::set<std::pair<std::string, std::string>> areAllNodesInPatternGraphUnique_;
};
