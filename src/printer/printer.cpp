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
        std::string typeString;
        if (dynamic_cast<ElementaryType*>(typeDecl.get()) != nullptr)
        {
            typeString = "int";
        }
        else
        {
            typeString = typeDecl->toString();
        }
        headerFile_ << "using " << typeDecl->identifier << " = " << typeString << ";" << std::endl;
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
        const std::string constType = constant->value->valueType->toString();
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
        const std::string varType = variable->value->valueType->toString();
        const std::string varValue = variable->value->toString();
        const std::string varName = variable->identifier;
        headerFile_ << varType << " " << varName << " = " << varValue << ";" << std::endl;
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
