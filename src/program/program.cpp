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

ReturnInstruction::ReturnInstruction()
{
}

ReturnInstruction::ReturnInstruction(const std::string &value) : value_(value)
{
}

std::string ReturnInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, "return" + (value_ == "" ? "" : " " + value_));
}

VariableDeclarationInstruction::VariableDeclarationInstruction(const std::string &name) :
    VariableDeclarationInstruction(name, "")
{
}

VariableDeclarationInstruction::VariableDeclarationInstruction(const std::string &name, std::string type) :
    name_(name), type_(type)
{
}

std::string VariableDeclarationInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, type_ + (type_ == "" ? "" : " ") + name_);
}

AssignmentInstruction::AssignmentInstruction(const std::string &left, const std::string &right) :
    AssignmentInstruction(left, right, "")
{
}

AssignmentInstruction::AssignmentInstruction(const std::string &left, const std::string &right, std::string type) :
    left_(std::make_unique<VariableDeclarationInstruction>(left, type)), right_(right)
{
}

std::string AssignmentInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, left_ -> toString(0, shift, false) + " = " + right_);
}

ComparisonInstruction::ComparisonInstruction(const std::string &left, const std::string &right) :
    ComparisonInstruction(left, right, false)
{
}

ComparisonInstruction::ComparisonInstruction(const std::string &left, const std::string &right, bool negated) :
    left_(left), right_(right), negated_(negated)
{
}

std::string ComparisonInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, left_ + (negated_ ? " != " : " == ") + right_);
}

IfInstruction::IfInstruction(std::unique_ptr<ComparisonInstruction> &&condition) : condition_(std::move(condition))
{
}

void IfInstruction::addInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

std::string IfInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;

    result += getLeadingSpaces(delimiter) + "if (" + condition_ -> toString(0, 0, false) + ")\n";
    result += getLeadingSpaces(delimiter) + "{\n";

    for (auto &&instruction : instructions_)
    {
        result += instruction -> toString(delimiter + shift, shift, true) + "\n";
    }

    result += getLeadingSpaces(delimiter) + "}\n";

    return result;
}

CustomInstruction::CustomInstruction(std::string instruction) : instruction_(instruction)
{
}

std::string CustomInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, instruction_);
}

Function::Function(std::string name, std::string returnType) : name_(name), returnType_(returnType)
{
}

void Function::addArgument(std::unique_ptr<VariableDeclarationInstruction> &&var)
{
    arguments_.push_back(std::move(var));
};

void Function::addInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

std::string Function::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;
    std::string argumentList;
    std::string body;

    for (auto & arg : arguments_)
    {
        if (arg -> toString(0, 0, false) != arguments_.back() -> toString(0, 0, false))
        {
            argumentList += arg -> toString(0, 0, false) + ", ";
        }
        else
        {
            argumentList += arg -> toString(0, 0, false);
        }
    }

    for (auto & instruction : instructions_)
    {
        body += instruction -> toString(shift, shift, true) + "\n";
    }

    result += getLeadingSpaces(delimiter) + returnType_ + " " + name_ + "(" + argumentList + ")\n";
    result += getLeadingSpaces(delimiter) + "{\n";
    result += body;
    result += getLeadingSpaces(delimiter) + "}\n";

    return result;
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

void Program::addFunction(std::unique_ptr<Function> &&function)
{
    functions_.push_back(std::move(function));
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

const std::vector<std::unique_ptr<Function>>& Program::getFunctions() const
{
    return functions_;
}
