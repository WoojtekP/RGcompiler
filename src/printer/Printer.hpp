#pragma once

#include <nlohmann/json.hpp>

#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

class Printer
{
public:
    Printer(
        const Parser& parse, const ValueAssigner& valueAssigner, std::ofstream& headerFile, std::ofstream& sourceFile);
    void initializeHeaderFile(bool debug);
    void initializeSourceFile();
    void initializeMainClass();
    void endMainClass();
    void endHeaderFile(std::string& hs, std::string& hs2);
    void endSourceFile();
    void printTypeDeclarations(const std::vector<std::shared_ptr<IType>>& typeDeclarations);
    void printSymbolValues();
    void printConstants(const std::vector<std::unique_ptr<IVariable>>& constans);
    void printVariables(const std::vector<std::unique_ptr<IVariable>>& variables, const std::string& xd);
    void printVariables(
        const std::vector<std::unique_ptr<IVariable>>& variables,
        bool isPublic,
        const std::string& prefix,
        const std::string& xd);
    void printFunctions(const std::vector<std::unique_ptr<Function>>& functions);
    void printMoveRepresentationDeclaration();
    void printAdditionDataForCycleHandling(const std::string& s);

private:
    const Parser& parser_;
    const ValueAssigner& valueAssigner_;
    std::ofstream& headerFile_;
    std::ofstream& sourceFile_;
};
