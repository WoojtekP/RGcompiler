#include <vector>

#include <program/program.hpp>


void Program::addTypeDeclaration(TypeDeclaration typeDecl)
{
    types.push_back(typeDecl);
}

void Program::addConstantDeclaration(ConstantDeclaration constantDecl)
{
    constants.push_back(constantDecl);
}

void Program::addVariableDeclaration(VariableDeclaration variableDecl)
{
    variables.push_back(variableDecl);
}

void Program::addFunction(Function function)
{
    functions.push_back(function);
}

std::vector<TypeDeclaration> Program::getTypes() const
{
    return types;
}

std::vector<ConstantDeclaration> Program::getConstants() const
{
    return constants;
}

std::vector<VariableDeclaration> Program::getVariables() const
{
    return variables;
}

std::vector<Function> Program::getFunctions() const
{
    return functions;
}
