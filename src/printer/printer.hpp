#pragma once

#include <parser/parser.hpp>
#include <program/program.hpp>

#include <nlohmann/json.hpp>


class Printer
{
public:
    Printer(const Parser& parse, std::ofstream& headerFile, std::ofstream& sourceFile);
    void initializeHeaderFile();
    void initializeSourceFile();
    void printTypeDeclarations(const std::vector<std::unique_ptr<IType>>& typeDeclarations);
    void printSymbolValues();
    void printConstants(const std::vector<std::unique_ptr<IVariable>>& constans);
    void printVariables(const std::vector<std::unique_ptr<IVariable>>& variables);
    void printStateChanges(const std::vector<std::unique_ptr<Function>> &functions);

private:
    void printIncludes();
    std::string typeToString(const nlohmann::json& t);
    std::string functionTypeToString(const nlohmann::json& functionType);
    void printConstants();
    void printGameState();
    void printVariables();
    std::string valueToString(const nlohmann::json& t, const nlohmann::json& value);
    std::string defaultValueToString(const nlohmann::json& t, const nlohmann::json& entries);

    const Parser& parser_;
    std::ofstream& headerFile_;
    std::ofstream& sourceFile_;
};
