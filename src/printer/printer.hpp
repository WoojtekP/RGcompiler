#pragma once

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>
#include <program/program.hpp>

class Printer
{
public:
    Printer(const Parser& parse, std::ofstream& headerFile, std::ofstream& sourceFile);
    void initializeHeaderFile();
    void initializeSourceFile();
    void initializeMainClass();
    void endMainClass();
    void endHeaderFile();
    void endSourceFile();
    void printTypeDeclarations(const std::vector<std::unique_ptr<IType>>& typeDeclarations);
    void printSymbolValues();
    void printConstants(const std::vector<std::unique_ptr<IVariable>>& constans);
    void printVariables(const std::vector<std::unique_ptr<IVariable>>& variables);
    void printFunctions(const std::vector<std::unique_ptr<Function>>& functions);
    void printMoveRepresentationDeclaration();

private:
    const Parser& parser_;
    std::ofstream& headerFile_;
    std::ofstream& sourceFile_;
};
