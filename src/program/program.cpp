#include <memory>
#include <vector>

#include <program/program.hpp>



std::string ElementaryType::toString() const
{
    return "int";
}

std::string FunctionType::toString() const
{
    std::string srcType = source->identifier;
    std::string dstType = destination->identifier;
    if (srcType.empty())
    {
        srcType = source->toString();
    }
    if (dstType.empty())
    {
        dstType = destination->toString();
    }
    return "std::map<" + srcType + ", " + dstType + ">";
}

void Program::addTypeDeclaration(std::unique_ptr<IType> typeDecl)
{
    types_.push_back(std::move(typeDecl));
}

void Program::addConstantDeclaration(ConstantDeclaration constantDecl)
{
    constants_.push_back(constantDecl);
}

void Program::addVariableDeclaration(VariableDeclaration variableDecl)
{
    variables_.push_back(variableDecl);
}

void Program::addFunction(Function function)
{
    functions_.push_back(function);
}

const std::vector<std::unique_ptr<IType>>& Program::getTypes() const
{
    return types_;
}

std::vector<ConstantDeclaration> Program::getConstants() const
{
    return constants_;
}

std::vector<VariableDeclaration> Program::getVariables() const
{
    return variables_;
}

std::vector<Function> Program::getFunctions() const
{
    return functions_;
}
