#include <vector>

#include <program/program.hpp>


void Program::addTypeDeclaration(TypeDeclaration typeDecl)
{
    types_.push_back(typeDecl);
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

std::vector<TypeDeclaration> Program::getTypes() const
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
