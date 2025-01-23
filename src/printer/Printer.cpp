#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/Parser.hpp>
#include <printer/Printer.hpp>
#include <program/Program.hpp>

namespace
{
bool isNumber(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), ::isdigit);
}
}  // namespace

Printer::Printer(
    const Parser& parser,
    const ValueAssigner& valueAssigner,
    const std::string& outputFileName,
    std::ofstream& headerFile,
    std::ofstream& sourceFile)
: parser_(parser)
, valueAssigner_(valueAssigner)
, outputFileName_(outputFileName)
, headerFile_(headerFile)
, sourceFile_(sourceFile)
{}

void Printer::initializeHeaderFile(bool debug)
{
    if (debug == 1)
    {
        headerFile_ << "#include <iostream>" << std::endl;
    }

    headerFile_ << "#include <array>" << std::endl;
    headerFile_ << "#include <bitset>" << std::endl;
    headerFile_ << "#include <set>" << std::endl;
    headerFile_ << "#include <string>" << std::endl;
    headerFile_ << "#include <tuple>" << std::endl;
    headerFile_ << "#include <unordered_map>" << std::endl;
    headerFile_ << "#include <unordered_set>" << std::endl;
    headerFile_ << "#include <vector>" << std::endl;
    headerFile_ << std::endl;
    headerFile_ << "#include <boost/container/static_vector.hpp>" << std::endl;
    headerFile_ << "#include <boost/container/small_vector.hpp>" << std::endl;

    headerFile_ << std::endl;
    headerFile_ << "namespace reasoner {" << std::endl;
    headerFile_ << "template<class T, std::size_t N>" << std::endl;
    headerFile_ << "using Arr = std::array<T, N>;" << std::endl;
    headerFile_ << "struct gameStateHasher;" << std::endl;

    headerFile_ << std::endl;
}

void Printer::initializeSourceFile()
{
    sourceFile_ << "#include \"" + outputFileName_ + ".hpp\"" << std::endl;
    sourceFile_ << std::endl;
    sourceFile_ << "#include <sstream>" << std::endl;
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

void Printer::printTypeDeclarations(const std::vector<std::shared_ptr<IType>>& typeDeclarations)
{
    for (const auto& typeDecl : typeDeclarations)
    {
        headerFile_ << "using " << typeDecl->identifier << " = " << typeDecl->definitionToString() << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printSymbolValues()
{
    std::set<std::string> printedSymbols;
    for (const auto& [typeName, symbolToValueMap] : valueAssigner_.getTypeToSymbolToValueMap())
    {
        std::map<int, std::string> valueToSymbols;
        for (const auto& [symbol, value] : symbolToValueMap)
        {
            if (!printedSymbols.count(symbol) && !isNumber(symbol))
            {
                valueToSymbols.emplace(value, symbol);
                printedSymbols.insert(symbol);
            }
        }
        for (const auto& [value, symbol] : valueToSymbols)
        {
            headerFile_ << "constexpr int " << symbol << " = " << value << ";" << std::endl;
        }
    }
    headerFile_ << std::endl;
}

void Printer::printConstants(const std::vector<std::unique_ptr<IVariable>>& constants)
{
    for (const auto& constant : constants)
    {
        const std::string constType = constant->valueType->toString();
        const std::string constValue = constant->value->toString(constant->valueType, valueAssigner_);
        const std::string constName = constant->identifier;
        headerFile_ << "constexpr " << constType << " " << constName << " = " << constValue << ";" << std::endl;
    }
    headerFile_ << std::endl;
}

void Printer::printVariables(
    const std::vector<std::unique_ptr<IVariable>>& variables,
    bool isPublic,
    const std::string& prefix)
{
    if (std::none_of(variables.begin(), variables.end(), [isPublic](const std::unique_ptr<IVariable>& f) {
            return f->isPublic() == isPublic;
        }))
    {
        return;
    }

    headerFile_ << prefix << std::endl;
    for (const auto& variable : variables)
    {
        if (variable->isPublic() == isPublic)
        {
            const std::string varType = variable->valueType->toString();
            const std::string varName = variable->identifier;
            if (variable->value)
            {
                const std::string varValue = variable->value->toString(variable->valueType, valueAssigner_);
                headerFile_ << varType << " " << varName << " = " << varValue << ";" << std::endl;
            }
            else
            {
                headerFile_ << varType << " " << varName << ";" << std::endl;
            }
        }
    }

    headerFile_ << std::endl;
}

void Printer::printVariables(const std::vector<std::unique_ptr<IVariable>>& variables)
{
    printVariables(variables, true, "public:");
    printVariables(variables, false, "private:");
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

void Printer::printNonGameStateFunctions(const std::vector<std::unique_ptr<Function>>& functions)
{
    for (const auto& f : functions)
    {
        headerFile_ << f->declarationToString() << std::endl;
        sourceFile_ << f->toString(0, 4, false) << std::endl;
    }
}

void Printer::printMoveRepresentationDeclaration(const std::pair<std::string, int>& moveRepresentation)
{
    const auto [moveContainer, moveSize] = moveRepresentation;
    const bool isArray = moveContainer.find("array") != std::string::npos;
    std::string moveInitialization = "move_representation mr";
    if (isArray)
    {
        moveInitialization += " = {";
        const std::string unusedTagValue = "-1";
        for (int i = 1; i < moveSize; ++i)
        {
            moveInitialization += unusedTagValue + ",";
        }
        moveInitialization += unusedTagValue + "}";
    }
    moveInitialization += ";";

    const auto structMoveDefinition = R"(
struct Move
{
)" + moveInitialization + R"(

    Move(void) = default;
    Move(const move_representation& mv) { mr = mv; }
    inline bool operator==(const Move& rhs) const { return mr == rhs.mr; }
    inline void reset() { )" + (isArray ? "mr.fill(-1);" : "mr.clear();") + R"(}
};)";

    const auto hashFunctions = R"(
namespace
{
inline void combine(size_t& acc, size_t x) noexcept
{
    acc ^= x;
}

inline size_t hash(int x) noexcept
{
    return x;
}

template<template<typename, size_t> class Container, typename T, size_t N>
size_t hash(const Container<T, N>& range) noexcept
{
    size_t acc = 0;
    for (const auto& x : range)
    {
        acc ^= hash(x);
    }
    return acc;
}
}  // namespace

struct move_hash
{
    size_t operator()(const move_representation& move) const noexcept
    {
        return hash(move);
    }
};)";

    headerFile_ << "using move_representation = " << moveContainer << "<int";
    if (moveSize != -1)
    {
        headerFile_ << "," << moveSize;
    }
    headerFile_ << ">;" << std::endl;

    headerFile_ << "class GameState;" << std::endl << std::endl;
    headerFile_ << "class RgCache;" << std::endl;
    headerFile_ << structMoveDefinition << std::endl << std::endl;
    headerFile_ << hashFunctions << std::endl << std::endl;
}

void Printer::printHashAndComparisonFunctions(const nlohmann::json& variables)
{
    const auto hasherDeclaration = R"(struct Hasher
    {
        size_t operator()(const std::tuple<GameState, int>& state) const noexcept;
        size_t operator()(const std::tuple<GameState, move_representation, int>& state) const noexcept;
    };)";
    const auto comparisonOperatorDeclaration = "bool operator==(const GameState& rhs) const;";

    std::string hashExpr = "hash(nodeId) ^ ";
    std::string cmpExpr = "";
    for (const auto& variable : variables)
    {
        const auto variableName = variable["identifier"].get<std::string>();
        hashExpr += "hash(gameState." + variableName + ") ^ ";
        cmpExpr += variableName + "==rhs." + variableName + " && ";
    }
    if (!cmpExpr.empty())
    {
        cmpExpr.resize(cmpExpr.size() - 4);
        hashExpr.resize(hashExpr.size() - 3);
    }

    headerFile_ << "public:" << std::endl;
    headerFile_ << hasherDeclaration << std::endl << std::endl;
    headerFile_ << comparisonOperatorDeclaration << std::endl;

    sourceFile_ << "size_t GameState::Hasher::operator()"
                << "(const std::tuple<GameState, int>& state) const noexcept" << std::endl;
    sourceFile_ << "{" << std::endl;
    sourceFile_ << "const auto& [gameState, nodeId] = state;" << std::endl;
    sourceFile_ << "return " << hashExpr << ";" << std::endl;
    sourceFile_ << "}" << std::endl << std::endl;

    sourceFile_ << "size_t GameState::Hasher::operator()"
                << "(const std::tuple<GameState, move_representation, int>& state) const noexcept" << std::endl;
    sourceFile_ << "{" << std::endl;
    sourceFile_ << "const auto& [gameState, move, nodeId] = state;" << std::endl;
    sourceFile_ << "return " << hashExpr << " ^ hash(move);" << std::endl;
    sourceFile_ << "}" << std::endl << std::endl;

    sourceFile_ << "bool GameState::operator==(const GameState& rhs) const" << std::endl;
    sourceFile_ << "{" << std::endl;
    sourceFile_ << "return " << cmpExpr << ";" << std::endl;
    sourceFile_ << "}" << std::endl << std::endl;
}

void Printer::printMainCache(const std::string& s)
{
    headerFile_ << s << std::endl << std::endl;
}
