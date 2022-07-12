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
    void generateFunctions();
    void generateStateFunctions(std::string type, std::function<std::string(std::string)> begining,
        std::function<std::string(std::string)> innerLoop, std::string ending);
    void generateEdgeFunctions(std::string type, std::string retVal, std::function<std::string(std::string)> middle,
        std::string ending);
    std::unique_ptr<IType> generateType(const nlohmann::json& t);
    std::unique_ptr<IType> generateFunctionType(const nlohmann::json& t);

    Parser& parser_;
    Graph graph_;
    Program program_;
};
