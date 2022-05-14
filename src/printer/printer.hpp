#pragma once

#include <parser/parser.hpp>

#include <nlohmann/json.hpp>


class Printer
{
public:
    Printer(const Parser& parse, std::ofstream& headerFile);
    void printHeaderFile();

private:
    void printIncludes();
    void printTypes();
    std::string typeToString(const nlohmann::json& t);
    std::string functionTypeToString(const nlohmann::json& functionType);
    void printConstants();
    void printGameState();
    void printVariables();

    const Parser& parser_;
    std::ofstream& headerFile_;
};
