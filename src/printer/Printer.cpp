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

    headerFile_ << std::endl;
    headerFile_ << "namespace reasoner {" << std::endl;
    headerFile_ << "template<class T, std::size_t N>" << std::endl;
    headerFile_ << "using Arr = std::array<T, N>;" << std::endl;
    headerFile_ << "struct hasher2;" << std::endl;

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

void Printer::endHeaderFile(std::string& hs, std::string& hs2)
{
    std::string ss = "struct hasher3{";
    ss += "size_t operator()(const std::tuple<GameState,move_representation,int>& gs) const{";
    ss += hs + "^ std::get<2>(gs)" + ";";
    ss += "}};";
    // std::string ss2 = "struct hasher2{";
    // ss2 += "size_t operator()(const GameState& gs) const{";
    // ss2 += "return " + hs2 + ";";
    // ss2 += "}};";
    std::string stateCacheDeclaration =
        "std::unordered_set<std::tuple<GameState,move_representation,int>, hasher3> state_cache;";

    //  program_.addVariableDeclaration(
    //  std::make_unique<Variable>("state_cache", std::move(std::make_shared<CustomType>(stateCacheDeclaration))));
    headerFile_ << "namespace {" + ss + stateCacheDeclaration + "}}  // namespace reasoner" << std::endl;
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
    const std::string& prefix,
    const std::string& xd)
{
    headerFile_ << prefix << std::endl;

    if (isPublic)
    {
        std::string ss = "struct hasher2{size_t operator()(const std::pair<GameState, int>& gs)const{";
        ss += "return " + xd + ";}};";
        headerFile_ << ss;
    }

    if (!std::accumulate(
            variables.begin(), variables.end(), false, [isPublic](bool acc, const std::unique_ptr<IVariable>& f) {
                return acc || (f->isPublic() == isPublic);
            }))
    {
        return;
    }

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

void Printer::printVariables(const std::vector<std::unique_ptr<IVariable>>& variables, const std::string& xd)
{
    printVariables(variables, true, "public:", xd);
    printVariables(variables, false, "private:", xd);
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
    std::string moveInitialization = "move_representation mr";
    if (moveContainer.find("array") != std::string::npos)
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
    Move(const move_representation& mv)
    {
        mr = mv;
    }
    bool operator==(const Move& rhs) const
    {
        return mr == rhs.mr;
    }
};)";

    const auto hashFunctions = R"(
namespace
{
void combine(size_t& acc, size_t x)
{
    acc ^= x;
}

template<typename T = int>
size_t hash(int x)
{
    return x;
}

template<typename T, size_t N>
size_t hash(std::array<T, N> a)
{
    size_t acc = 0;
    for (size_t i = 0; i < N; i++)
    {
        combine(acc, hash(a[i]));
    }

    return acc;
}

template <typename T, size_t N>
size_t hash(const boost::container::static_vector<T, N> &v)
{
    size_t acc = 0;
    for (auto x : v)
    {
        acc ^= x;
    }
    return acc;
}

[[maybe_unused]] size_t hash(const std::vector<int>& v)
{
    size_t x = 0;
    for (int t : v)
    {
        x ^= t;
    }
    return x;
}
}  // namespace)";

    headerFile_ << "using move_representation = " << moveContainer << "<int";
    if (moveSize != -1)
    {
        headerFile_ << "," << moveSize;
    }
    headerFile_ << ">;" << std::endl;

    headerFile_ << "class GameState;" << std::endl << std::endl;
    headerFile_ << structMoveDefinition << std::endl << std::endl;
    headerFile_ << hashFunctions << std::endl << std::endl;
}

void Printer::printAdditionDataForCycleHandling(const std::string& s)
{
    headerFile_ << s << std::endl << std::endl;
}
