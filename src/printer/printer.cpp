#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>
#include <printer/printer.hpp>
#include <program/program.hpp>


Printer::Printer(const Parser& parser, std::ofstream& headerFile, std::ofstream& sourceFile)
: parser_(parser)
, headerFile_(headerFile)
, sourceFile_(sourceFile)
{
}

void Printer::initializeHeaderFile()
{
    headerFile_ << "#include <map>" << std::endl;
    headerFile_ << std::endl;
}

void Printer::printTypeDeclarations(const std::vector<std::unique_ptr<IType>>& typeDeclarations)
{
    for (const auto& typeDecl : typeDeclarations)
    {
        headerFile_ << "using " << typeDecl->identifier << " = " << typeDecl->toString() << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printSymbolValues()
{
    for (const auto& [typeIdentifier, symbolAndValues] : parser_.getTypeToSymbolsAndValuesMap())
    {
        for (const auto& [symbol, value] : symbolAndValues)
        {
            headerFile_ << "constexpr " << typeIdentifier << " " << typeIdentifier + "_" + symbol << " = " << value << ";" << std::endl;
        }
    }
    headerFile_ << std::endl;
}

void Printer::printStateChanges(const std::vector<std::unique_ptr<Function>> &functions)
{
    for (const auto &f : functions)
    {
        sourceFile_ << f -> toString(0,4,false) << "\n";
    }
}

std::string Printer::typeToString(const nlohmann::json& t)
{
    if (t.is_string())
    {
        return t;
    }
    else if (t["kind"] == "TypeReference")
    {
        return t["identifier"];
    }
    else if (t["kind"] == "Arrow")
    {
        return functionTypeToString(t);
    }
    return "???";
}

std::string Printer::functionTypeToString(const nlohmann::json& functionType)
{
    return "std::map<" + typeToString(functionType["lhs"]) + ", " + typeToString(functionType["rhs"]) + ">";
}

void Printer::printConstants()
{
    for (const auto& constant : parser_.getConstants())
    {
        const std::string constType = typeToString(constant["type"]);
        const std::string constName = constant["identifier"];
        const std::string constValue =  valueToString(constant["type"], constant["value"]);
        headerFile_ << "const " << constType << " " << constName << " = " << constValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printVariables()
{
    for (const auto& variable : parser_.getVariables())
    {
        const std::string varType = typeToString(variable["type"]);
        const std::string varName = variable["identifier"];
        const std::string varValue =  valueToString(variable["type"], variable["defaultValue"]);
        headerFile_ << varType << " " << varName << " = " << varValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

std::string Printer::valueToString(const nlohmann::json& t, const nlohmann::json& value)
{
    if (value["kind"] == "Element")
    {
        return parser_.getValue(value["identifier"]);
    }
    else if (value["kind"] == "Map")
    {
        std::map<std::string, std::string> identifierToValue;
        const auto destinationType = parser_.getDestinationType(t);
        for (const auto& entry : value["entries"])
        {
            if (entry["kind"] == "NamedEntry")
            {
                identifierToValue.emplace(entry["identifier"], valueToString(destinationType, entry["value"]));
            }
        }
        const auto defaultValue = defaultValueToString(t, value["entries"]);
        const auto sourceType = parser_.getSourceType(t);
        for (const auto& identifier : parser_.getDomain(sourceType))
        {
            if (identifierToValue.find(identifier) == identifierToValue.end())
            {
                identifierToValue.emplace(identifier, defaultValue);
            }
        }
        std::string result = "{";
        int i = 1;
        for (const auto& [id, val] : identifierToValue)
        {
            result += "{" + parser_.getValue(id) + ", " + val + "}";
            if (i < identifierToValue.size())
            {
                result += ", ";
            }
            ++i;
        }
        return result + "}";
    }
    return "?";
}

std::string Printer::defaultValueToString(const nlohmann::json& t, const nlohmann::json& entries)
{
    for (const auto& entry : entries)
    {
        if (entry["kind"] == "DefaultEntry")
        {
            return valueToString(t, entry["value"]);
        }
    }
    return "?";
}

void Printer::printGameState()
{
    headerFile_ << "class Reasoner" << std::endl;
    headerFile_ << "{" << std::endl;
    printVariables();
    headerFile_ << "};" << std::endl;
}
