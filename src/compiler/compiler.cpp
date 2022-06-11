#include <compiler/compiler.hpp>
#include <parser/parser.hpp>
#include <printer/printer.hpp>


Compiler::Compiler(Parser& parser) : parser_(parser)
{
}

void Compiler::compile()
{
    generateTypes();
    // TODO: transform graph into program
}

void Compiler::generateSourceCode(std::ofstream& headerFile, std::ofstream& sourceFile)
{
    Printer printer(parser_, headerFile, sourceFile);
    printer.initializeHeaderFile();
    printer.printTypeDeclarations(program_.getTypes());

    printer.printStateChanges();
    // TODO: transform program into C++ source code using printer
}

void Compiler::generateTypes()
{
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] != "Arrow")
        {
            auto newElementaryType = std::make_unique<ElementaryType>();
            newElementaryType->identifier = t["identifier"].get<std::string>();
            program_.addTypeDeclaration(std::move(newElementaryType));
        }
    }
    for (const auto& t : parser_.getTypeDeclarations())
    {
        if (t["type"]["kind"] == "Arrow")
        {
            auto newFunctionType = generateType(t["type"]);
            newFunctionType->identifier = t["identifier"].get<std::string>();
            program_.addTypeDeclaration(std::move(newFunctionType));
        }
    }
}

std::unique_ptr<IType> Compiler::generateType(const nlohmann::json& t)
{
    if (t.is_string())
    {
        return std::make_unique<ElementaryType>(t.get<std::string>());
    }
    else if (t["kind"] == "TypeReference")
    {
        return std::make_unique<ElementaryType>(t["identifier"].get<std::string>());
    }
    else if (t["kind"] == "Arrow")
    {
        return generateFunctionType(t);
    }
    return nullptr;
}

std::unique_ptr<IType> Compiler::generateFunctionType(const nlohmann::json& functionType)
{
    return std::make_unique<FunctionType>(generateType(functionType["lhs"]), generateType(functionType["rhs"]));
}
