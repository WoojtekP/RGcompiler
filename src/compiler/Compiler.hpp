#pragma once

#include <compiler/ValueAssigner.hpp>
#include <compiler/cacheContainers/ContainerChooser.hpp>
#include <graph/ActionFactory.hpp>
#include <graph/Edge.hpp>
#include <graph/Graph.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

struct Options
{
    int debug;
    int optConditions;
    bool simplePathCompression_;
    bool noCycleDetection_;
};

class Compiler
{
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
        const std::string &from, const std::string &to, const std::shared_ptr<Graph> &graph, int patternId = 0);
    std::unique_ptr<BlockInstruction> addActionPattern(
        const std::shared_ptr<IAction> &action,
        const std::shared_ptr<Graph> &graph,
        const std::string &stateFrom,
        const std::string &stateTo,
        int iid,
        std::unique_ptr<BlockInstruction> blockInstruction);
    std::unique_ptr<BlockInstruction> generateVoidEdgeInstruction(
        const std::shared_ptr<Graph> &graph, const std::shared_ptr<Edge> &edge, int iid, bool applyEdgeMode = false);
    std::unique_ptr<BlockInstruction> prepareBaseInstructions(
        const std::shared_ptr<Graph> &graph,
        std::vector<std::shared_ptr<IAction>> &actions,
        const std::shared_ptr<Edge> &edge,
        int iid,
        bool applyEdgeMode);
    std::unique_ptr<BlockInstruction> generateBoolEdgeInstruction(
        const std::string &from,
        const std::string &to,
        const std::shared_ptr<Graph> &graph,
        const std::string &stateFrom,
        const std::string &stateTo,
        int iid,
        int patternId);
    std::unique_ptr<BlockInstruction> prepareBaseInstructions(
        const std::shared_ptr<Graph> &graph,
        const std::vector<std::shared_ptr<IAction>> &actions,
        const std::string &stateFrom,
        const std::string &stateTo,
        int iid,
        const std::string &cacheName,
        const std::string &prefix,
        int patternId);
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
    std::shared_ptr<IType> generateType(const nlohmann::json &t);
    std::shared_ptr<IType> generateFunctionType(const nlohmann::json &t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json &value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json &value);
    std::unique_ptr<IInstruction> debugInstruction(std::string functionName);
    std::string getTemporaryVariableName(int idx, int edgeId);
    int getNumberOfPlayers();
    int getDomain(const std::string &s);

    const Parser &parser_;
    const ValueAssigner valueAssigner_;
    std::shared_ptr<Graph> graph_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternReachabilityGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternAnyGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> applyAnyMoveGraphs_;
    std::map<std::tuple<std::string, std::string, int>, std::set<std::string>> variablesInPatternGraphs_;
    Program program_;
    const std::string temporaryVariableNamePrefix_;
    const int debugFlag_;
    const bool optConditionsReachability_;
    const bool optConditionsGeneratingMoves_;
    const bool optConditionsSimplePathCompression_;
    const bool optNoCycleDetection_;
    const std::string patternIdToPrefixName[3] = {"", "any_", "any2_"};
    const std::string mainCacheName_;
    const std::string mainCacheType_;
    ContainerChooser containerChooser_;
};
