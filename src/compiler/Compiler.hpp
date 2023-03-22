#pragma once

#include <compiler/cacheContainers/ContainerChooser.hpp>
#include <compiler/ValueAssigner.hpp>
#include <graph/ActionFactory.hpp>
#include <graph/Graph.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

struct Options
{
    bool debug;
    int optConditions;
    bool simplePathCompression_;
    bool moveCompression_;
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
    void generateVoidStateFunctions(const std::shared_ptr<Graph> &graph);
    void generateBoolStateFunctions(
        const std::string &from, const std::string &to, const std::shared_ptr<Graph> &graph, int patternId = 0);
    void generateVoidEdgeFunctions(const std::shared_ptr<Graph> &graph);
    void generateBoolEdgeFunctions(
        const std::string &from, const std::string &to, const std::shared_ptr<Graph> &graph, int patternId);
    void generateApplyEdgeFunctions(const std::shared_ptr<Graph> &graph);
    void generateSpecialFunctions(const std::shared_ptr<Graph> &graph);
    void generateRunApplyEdgeFunction(const std::shared_ptr<Graph> &graph);
    void generateRunStateFunction(const std::shared_ptr<Graph> &graph);
    void generateGetFromStateForEdge(const std::shared_ptr<Graph> &graph);
    void generateVoidStateOptimizedFunction(
        const std::string &state, const std::unique_ptr<Function> &function, const std::shared_ptr<Graph> &graph);
    void generatePatternFunctions(
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, int patternId = 0);
    void generatePatternReachabilityFunctions();
    void generatePatternAnyFunctions();
    void generateApplyAnyMove();
    void initializePatternGraphs(
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> &patterns, int patternId = 0);
    template<typename T>
    void restoreAssignments(const std::unique_ptr<T> &function, std::vector<std::shared_ptr<IAction>> assignments);
    std::string getStateIntId(std::string name);
    std::shared_ptr<IType> generateType(const nlohmann::json &t);
    std::shared_ptr<IType> generateFunctionType(const nlohmann::json &t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json &value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json &value);
    std::unique_ptr<IInstruction> debugInstruction(std::string functionName);
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
    const bool debugFlag_;
    const bool optConditionsReachability_;
    const bool optConditionsGeneratingMoves_;
    const bool optConditionsSimplePathCompression_;
    const bool optConditionsMoveCompression_;
    const bool optNoCycleDetection_;
    const std::string patternIdToPrefixName[3] = {"", "any_", "any2_"};
    const std::string mainCacheName_;
    const std::string mainCacheType_;
    ContainerChooser containerChooser_;
};
