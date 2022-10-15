#pragma once

#include <graph/graph.hpp>
#include <parser/parser.hpp>
#include <program/program.hpp>


struct Options
{
    bool debug;
    int optConditions;
};

class Compiler
{
public:
    Compiler(Parser& parser, const Options& options);
    void compile();
    void generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile);

private:
    void initializeGraph();
    void generateTypes();
    void generateConstants();
    void generateVariables();
    void generateFunctions();
    void generateVoidStateFunctions();
    void generateVoidStateOptimizedFunction(const std::string& state, const std::unique_ptr<Function>& function);
    void generateBoolStateFunctions();
    void generateVoidEdgeFunctions();
    void generateBoolEdgeFunctions();
    void generateApplyEdgeFunctions();
    void generateSpecialFunctions();
    void generateIntRepresentationForStates();
    void generateIntRepresentationForEdges();
    void generateRunApplyEdgeFunction();
    void generateRunStateFunction();
    void generateGetFromStateForEdge();
    std::string getStateIntId(std::string name);
    std::shared_ptr<IType> generateType(const nlohmann::json& t);
    std::shared_ptr<IType> generateFunctionType(const nlohmann::json& t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json& value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json& value);
    std::unique_ptr<IInstruction> debugInstruction(std::string functionName);

    std::map<std::string, int> stateStringToInt_;
    std::map<std::pair<std::string, std::string>, int> edgeStringToInt_;
    Parser& parser_;
    Graph graph_;
    Program program_;
    const bool debugFlag_;
    const bool optConditionsReachability_;
    const bool optConditionsGeneratingMoves_;
};
