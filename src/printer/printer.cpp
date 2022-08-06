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
    headerFile_ << "#include <vector>" << std::endl;
    headerFile_ << "#include <string>" << std::endl;
    headerFile_ << std::endl;
    headerFile_ << "#include \"defaultMap.hpp\"" << std::endl;
    headerFile_ << std::endl;
}

void Printer::initializeSourceFile()
{
    sourceFile_ << "#include \"reasoner.hpp\"" << std::endl;
    sourceFile_ << std::endl;
}

void Printer::printTypeDeclarations(const std::vector<std::unique_ptr<IType>>& typeDeclarations)
{
    for (const auto& typeDecl : typeDeclarations)
    {
        headerFile_ << "using " << typeDecl->identifier << " = " << typeDecl->definitionToString() << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printSymbolValues()
{
    for (const auto& [symbol, value] : parser_.getSymbolToValueMap())
    {
        headerFile_ << "constexpr int " << symbol << " = " << value << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printConstants(const std::vector<std::unique_ptr<IVariable>>& constants)
{
    for (const auto& constant : constants)
    {
        const std::string constType = constant->valueType->toString();
        const std::string constValue = constant->value->toString();
        const std::string constName = constant->identifier;
        headerFile_ << "const " << constType << " " << constName << " = " << constValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printVariables(const std::vector<std::unique_ptr<IVariable>>& variables)
{
    for (const auto& variable : variables)
    {
        const std::string varType = variable->valueType->toString();
        const std::string varName = variable->identifier;
        if (variable->value)
        {
            const std::string varValue = variable->value->toString();
            headerFile_ << varType << " " << varName << " = " << varValue << ";" << std::endl;
        }
        else
        {
            headerFile_ << varType << " " << varName << ";" << std::endl;
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

void Printer::printMainClass(std::string obj, const std::vector<std::string> &functions)
{
    headerFile_ << obj << std::endl;

    sourceFile_ << "void game_state::game_state()\n{\n";
    for (auto &name : functions)
    {
        sourceFile_ << "    nameToFunction[\"" << name << "\"] = " << name << ";\n";
    }
    sourceFile_ << "}\n";
}
