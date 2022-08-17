#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>
#include <printer/printer.hpp>
#include <program/program.hpp>

Printer::Printer(const Parser& parser, std::ofstream& headerFile, std::ofstream& sourceFile)
: parser_(parser), headerFile_(headerFile), sourceFile_(sourceFile)
{}

void Printer::initializeHeaderFile(bool debug)
{
    if (debug)
    {
        headerFile_ << "#include <iostream>" << std::endl;
    }
    headerFile_ << "#include <vector>" << std::endl;
    headerFile_ << "#include <string>" << std::endl;
    headerFile_ << std::endl;
    headerFile_ << "#include \"defaultMap.hpp\"" << std::endl;
    headerFile_ << std::endl;
    headerFile_ << "namespace reasoner {" << std::endl;
}

void Printer::initializeSourceFile()
{
    sourceFile_ << "#include \"reasoner.hpp\"" << std::endl;
    sourceFile_ << std::endl;
    sourceFile_ << "namespace reasoner {" << std::endl;
}

void Printer::initializeMainClass()
{
    headerFile_ << "class GameState" << std::endl;
    headerFile_ << "{" << std::endl;
}

void Printer::endMainClass()
{
    headerFile_ << "};" << std::endl;
}

void Printer::endHeaderFile()
{
    headerFile_ << "}  // namespace reasoner" << std::endl;
}

void Printer::endSourceFile()
{
    sourceFile_ << "}  // namespace reasoner" << std::endl;
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
    headerFile_ << "private:" << std::endl;
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

void Printer::printFunctions(const std::vector<std::unique_ptr<Function>>& functions)
{
    headerFile_ << "public:" << std::endl;
    for (const auto& f : functions)
    {
        if (f->isPublic())
        {
            headerFile_ << f->declarationToString() << std::endl;
            sourceFile_ << f->toString(0, 4, false) << std::endl;
        }
    }

    headerFile_ << "private:" << std::endl;
    for (const auto& f : functions)
    {
        if (!f->isPublic())
        {
            headerFile_ << f->declarationToString() << std::endl;
            sourceFile_ << f->toString(0, 4, false) << std::endl;
        }
    }
}

void Printer::printMoveRepresentationDeclaration()
{
    std::string obj = R"(
class GameState;

typedef std::vector<int> move_representation;
typedef void(GameState::*funcPtr)();

struct Move
{
    move_representation mr;

    Move(void) = default;
    Move(const move_representation& mv)
    {
        mr.assign(mv.begin(), mv.end());
    }
    bool operator==(const Move& rhs) const
    {
        return mr == rhs.mr;
    }
};)";

    headerFile_ << obj << std::endl << std::endl;
}
