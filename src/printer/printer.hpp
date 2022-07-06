#pragma once

#include <parser/parser.hpp>
#include <program/program.hpp>

#include <nlohmann/json.hpp>


class Printer
{
public:
    Printer(const Parser& parse, std::ofstream& headerFile, std::ofstream& sourceFile);
    void initializeHeaderFile();
    void printTypeDeclarations(const std::vector<std::unique_ptr<IType>>& typeDeclarations);
    void printSymbolValues();
    void printStateChanges();

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
