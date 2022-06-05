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
    std::string valueToString(const nlohmann::json& t, const nlohmann::json& value);
    std::string defaultValueToString(const nlohmann::json& t, const nlohmann::json& entries);

    const Parser& parser_;
    std::ofstream& headerFile_;
};
