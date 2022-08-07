#pragma once

#include <graph/graph.hpp>
#include <parser/parser.hpp>
#include <program/program.hpp>


class Compiler
{
public:
    Compiler(Parser& parser);
    void compile();
    void generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile);

private:
    void initializeGraph();
    void generateTypes();
    void generateConstants();
    void generateVariables();
    void generateFunctions();
    void generateVoidStateFunctions();
    void generateBoolStateFunctions();
    void generateVoidEdgeFunctions();
    void generateBoolEdgeFunctions();
    void generateApplyEdgeFunctions();
    void generateSpecialFunctions();
    std::unique_ptr<IType> generateType(const nlohmann::json& t);
    std::unique_ptr<IType> generateFunctionType(const nlohmann::json& t);
    std::unique_ptr<IValue> generateValue(const nlohmann::json& value);
    std::unique_ptr<IValue> generateMapValue(const nlohmann::json& value);

    Parser& parser_;
    Graph graph_;
    Program program_;
};
