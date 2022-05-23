#pragma once

#include <parser/parser.hpp>

#include <nlohmann/json.hpp>


class Printer
{
public:
    Printer(const Parser& parse, std::ofstream& headerFile, std::ofstream& sourceFile);
    void printHeaderFile();
    void printSourceFile();

private:
    void printIncludes();
    void printTypes();
    std::string typeToString(const nlohmann::json& t);
    std::string functionTypeToString(const nlohmann::json& functionType);
    void printConstants();
    void printGameState();
    void printVariables();
    void printStateChanges();
    std::string valueToString(const nlohmann::json& t, const nlohmann::json& value);
    std::string defaultValueToString(const nlohmann::json& t, const nlohmann::json& entries);

    const Parser& parser_;
    std::ofstream& headerFile_;
    std::ofstream& sourceFile_;
};
