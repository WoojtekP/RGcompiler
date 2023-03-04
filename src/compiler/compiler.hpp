#pragma once

#include <compiler/valueAssigner.hpp>
#include <graph/graph.hpp>
#include <graph/actionFactory.hpp>
#include <parser/parser.hpp>
#include <program/program.hpp>

struct Options
{
    bool debug;
    int optConditions;
    bool simplePathCompression_;
    bool moveCompression_;
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
    std::tuple<std::string, std::function<std::string(std::string)>, std::function<std::string(std::string)>>
    getExecutionTypesForCyclicStates(const std::map<std::string, int> &m);

    const Parser &parser_;
    const ValueAssigner valueAssigner_;
    std::shared_ptr<Graph> graph_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternReachabilityGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternAnyGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> applyAnyMoveGraphs_;
    std::map<std::tuple<std::string, std::string, int>, std::map<std::string, int>> variablesInPatternGraphs_;
    Program program_;
    const std::string temporaryVariableNamePrefix_;
    const bool debugFlag_;
    const bool optConditionsReachability_;
    const bool optConditionsGeneratingMoves_;
    const bool optConditionsSimplePathCompression_;
    const bool optConditionsMoveCompression_;
    const std::string patternIdToPrefixName[3] = {"", "any_", "any2_"};
};
