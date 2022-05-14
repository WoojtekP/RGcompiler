#include <iostream>
#include <fstream>

#include <printer/printer.hpp>


Printer::Printer(const Parser& parser, std::ofstream& headerFile)
: parser_(parser)
, headerFile_(headerFile)
{
}

void Printer::printHeaderFile()
{
    printIncludes();
    printTypes();
    printConstants();
    printGameState();
}

void Printer::printIncludes()
{
    headerFile_ << "#include <map>" << std::endl;
    headerFile_ << std::endl;
}

void Printer::printTypes()
{
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] != "Arrow")
        {
            headerFile_ << "using " << t["identifier"].get<std::string>() << " = int;" << std::endl;
        }
    }
    headerFile_ << std::endl;
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] == "Arrow")
        {
            headerFile_ << "using " << t["identifier"].get<std::string>() << " = " << typeToString(t["type"]) << ";" << std::endl;
        }
    }
    headerFile_ << std::endl;
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
    // TODO
}

void Printer::printVariables()
{
    // TODO
}

void Printer::printGameState()
{
    headerFile_ << "class Reasoner" << std::endl;
    headerFile_ << "{" << std::endl;
    printVariables();
    headerFile_ << "};" << std::endl;
}
