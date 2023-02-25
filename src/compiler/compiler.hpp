#pragma once

#include <graph/graph.hpp>
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
    Compiler(Parser &parser, const Options &options);
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
        const std::string &from, const std::string &to, const std::shared_ptr<Graph> &graph, bool patternAny = false);
    void generateVoidEdgeFunctions(const std::shared_ptr<Graph> &graph);
    void generateBoolEdgeFunctions(
        const std::string &from, const std::string &to, const std::shared_ptr<Graph> &graph, bool patternAny);
    void generateApplyEdgeFunctions(const std::shared_ptr<Graph> &graph);
    void generateSpecialFunctions(const std::shared_ptr<Graph> &graph);
    void generateRunApplyEdgeFunction(const std::shared_ptr<Graph> &graph);
    void generateRunStateFunction(const std::shared_ptr<Graph> &graph);
    void generateGetFromStateForEdge(const std::shared_ptr<Graph> &graph);
    void generateVoidStateOptimizedFunction(
        const std::string &state, const std::unique_ptr<Function> &function, const std::shared_ptr<Graph> &graph);
    void generatePatternFunctions(
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patterns, bool patternAny = false);
    void generatePatternReachabilityFunctions();
    void generatePatternAnyFunctions();
    void generateApplyAnyMove();
    void addNodesForApplyAnyMoveTopatternAnyGraphs();
    void initializePatternGraphs(std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> &patterns);
    template<typename T>
    void restoreAssignments(const std::unique_ptr<T> &function, std::vector<std::shared_ptr<Action>> assignments);
    std::string getStateIntId(std::string name);
    std::shared_ptr<IType> generateType(const nlohmann::json &t);
    std::shared_ptr<IType> generateFunctionType(const nlohmann::json &t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json &value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json &value);
    std::unique_ptr<IInstruction> debugInstruction(std::string functionName);

    Parser &parser_;
    std::shared_ptr<Graph> graph_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternReachabilityGraphs_;
    std::vector<std::tuple<std::string, std::string, std::shared_ptr<Graph>>> patternAnyGraphs_;
    std::vector<std::pair<std::string, std::vector<std::string>>> nodesForApplyAnyMove_;
    Program program_;
    const std::string temporaryVariableNamePrefix_;
    const bool debugFlag_;
    const bool optConditionsReachability_;
    const bool optConditionsGeneratingMoves_;
    const bool optConditionsSimplePathCompression_;
    const bool optConditionsMoveCompression_;
};
