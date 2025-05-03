#include "Program.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include <compiler/ValueAssigner.hpp>
#include <program/Program.hpp>

std::string ElementaryType::toString() const
{
    return identifier;
}

std::string ElementaryType::definitionToString() const
{
    return "int";
}

std::string FunctionType::toString() const
{
    if (identifier.empty())
    {
        return definitionToString();
    }
    return identifier;
}

std::string FunctionType::definitionToString() const
{
    return "Arr<" + destination->toString() + ", " + std::to_string(domainSize) + ">";
}

std::string CustomType::toString() const
{
    return identifier;
}

std::string CustomType::definitionToString() const
{
    return typeDefinition;
}

std::string SingleValue::toString(const std::shared_ptr<IType> &, const ValueAssigner &) const
{
    return symbol;
}

std::string MapValue::toString(const std::shared_ptr<IType> &t, const ValueAssigner &valueAssigner) const
{
    if (const FunctionType *functionType = dynamic_cast<FunctionType *>(t.get()))
    {
        const std::string sourceTypeName = functionType->source->identifier;
        const auto [minValue, maxValue] = valueAssigner.getTypeMinMaxValues(sourceTypeName);
        const auto &symbolToValueMap = valueAssigner.getTypeToSymbolToValueMap().at(sourceTypeName);
        const std::string defaultValueString =
            (defaultValue ? defaultValue->toString(functionType->destination, valueAssigner) : "?");
        const auto size = maxValue - minValue + 1;
        std::vector<std::string> values(size, defaultValueString);
        for (const auto &[id, value] : idToValueMap)
        {
            const int pos = symbolToValueMap.at(id);
            values[pos - minValue] = value->toString(functionType->destination, valueAssigner);
        }
        std::string result = functionType->toString() + "{";
        for (const auto &value : values)
        {
            result += value + ",";
        }
        result += "}";
        return result;
    }
    throw std::invalid_argument("Function type is expected.");
}

std::string Constant::toString() const
{
    return identifier;
}

std::string Variable::toString() const
{
    return identifier;
}

ReturnInstruction::ReturnInstruction() {}

ReturnInstruction::ReturnInstruction(const std::string &value)
: value_(value)
{}

std::string ReturnInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, "return" + (value_ == "" ? "" : " " + value_));
}

VariableDeclarationInstruction::VariableDeclarationInstruction(const std::string &name)
: VariableDeclarationInstruction(name, "")
{}

VariableDeclarationInstruction::VariableDeclarationInstruction(const std::string &name, std::string type)
: name_(name)
, type_(type)
{}

std::string VariableDeclarationInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, type_ + (type_ == "" ? "" : " ") + name_);
}

AssignmentInstruction::AssignmentInstruction(const std::string &left, const std::string &right)
: AssignmentInstruction(left, right, "")
{}

AssignmentInstruction::AssignmentInstruction(const std::string &left, const std::string &right, std::string type)
: left_(std::make_unique<VariableDeclarationInstruction>(left, type))
, right_(right)
{}

std::string AssignmentInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, left_->toString(0, shift, false) + " = " + right_);
}

std::string cmpToString(const ComparisonType cmpType)
{
    switch (cmpType)
    {
        case ComparisonType::None:
            return "";
        case ComparisonType::Neg:
            return "!";
        case ComparisonType::Eq:
            return "==";
        case ComparisonType::Neq:
            return "!=";
        case ComparisonType::Gr:
            return ">";
        case ComparisonType::Ge:
            return ">=";
        case ComparisonType::Less:
            return "<";
        case ComparisonType::Leq:
            return "<=";
    }
    throw std::invalid_argument("[Program] Unkwnon ComparisonType: " + std::to_string(static_cast<int>(cmpType)));
}

ComparisonInstruction::ComparisonInstruction(const std::string &expr, ComparisonType cmp)
: left_(expr)
, cmpType_(cmp)
{
    if (cmpType_ != ComparisonType::None && cmpType_ != ComparisonType::Neg)
    {
        throw std::invalid_argument(
            "[Program] ComparisonInstruction with single expression \"" + left_ +
            "\" created with invalid type: " + cmpToString(cmpType_));
    }
}

ComparisonInstruction::ComparisonInstruction(const std::string &left, const std::string &right, ComparisonType cmp)
: left_(left)
, right_(right)
, cmpType_(cmp)
{
    if (cmpType_ == ComparisonType::None || cmpType_ == ComparisonType::Neg)
    {
        throw std::invalid_argument(
            "[Program] ComparisonInstruction with two expressions \"" + left_ + "\" and \"" + right_ +
            "\" created with invalid type: " + cmpToString(cmpType_));
    }
}

ComparisonType ComparisonInstruction::getType() const
{
    return cmpType_;
}

std::pair<std::string, std::string> ComparisonInstruction::getSubexpressions() const
{
    return {left_, right_};
}

std::string ComparisonInstruction::toString(int delimiter, int shift, bool semicolon)
{
    if (cmpType_ == ComparisonType::None || cmpType_ == ComparisonType::Neg)
    {
        return addSpacesAndSemicolon(delimiter, semicolon, cmpToString(cmpType_) + left_);
    }

    return addSpacesAndSemicolon(delimiter, semicolon, left_ + cmpToString(cmpType_) + right_);
}

IfInstruction::IfInstruction(std::unique_ptr<ComparisonInstruction> &&condition)
: condition_(std::move(condition))
{}

std::vector<std::unique_ptr<IInstruction>> IfInstruction::extractInstructions()
{
    return std::move(instructions_);
}

const std::unique_ptr<ComparisonInstruction> &IfInstruction::getCondition() const
{
    return condition_;
}

void IfInstruction::addInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

void IfInstruction::addElseInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    elseInstruction_ = std::move(instruction);
}

std::string IfInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;

    result += getLeadingSpaces(delimiter) + "if (" + condition_->toString(0, 0, false) + ")\n";
    result += getLeadingSpaces(delimiter) + "{\n";

    for (auto &&instruction : instructions_)
    {
        result += instruction->toString(delimiter + shift, shift, true) + "\n";
    }

    result += getLeadingSpaces(delimiter) + "}\n";

    if (elseInstruction_)
    {
        if (dynamic_cast<IfInstruction *>(elseInstruction_.get()) != nullptr)
        {
            result += getLeadingSpaces(delimiter) + "else ";
            result += elseInstruction_->toString(delimiter, shift, true);
            result += getLeadingSpaces(delimiter) + "\n";
        }
        else
        {
            result += getLeadingSpaces(delimiter) + "else {\n";
            result += elseInstruction_->toString(delimiter + shift, shift, true);
            result += getLeadingSpaces(delimiter) + "\n" + getLeadingSpaces(delimiter) + "}\n";
        }
    }
    result.pop_back();
    return result;
}

SwitchInstruction::SwitchInstruction(const std::string &condition)
: condition_(condition)
{}

void SwitchInstruction::addCaseInstruction(int valMin, std::unique_ptr<IInstruction> &&instruction, bool breakAfter)
{
    instructions_.push_back({std::make_pair(valMin, valMin), std::move(instruction), breakAfter});
}

void SwitchInstruction::addCaseInstruction(
    int valMin, int valMax, std::unique_ptr<IInstruction> &&instruction, bool breakAfter)
{
    instructions_.push_back({std::make_pair(valMin, valMax), std::move(instruction), breakAfter});
}

void SwitchInstruction::addDefaultInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    default_ = std::move(instruction);
}

std::string SwitchInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;

    result += getLeadingSpaces(delimiter) + "switch (" + condition_ + ")\n";
    result += getLeadingSpaces(delimiter) + "{\n";

    int newDelimiter = delimiter + shift;
    for (const auto &[valPair, instruction, breakAfter] : instructions_)
    {
        auto [valMin, valMax] = valPair;
        std::string caseVal;
        if (valMin == valMax)
        {
            caseVal = std::to_string(valMin);
        }
        else
        {
            caseVal = std::to_string(valMin) + " ... " + std::to_string(valMax);
        }
        result += getLeadingSpaces(newDelimiter) + "case " + caseVal + ":\n{\n";
        result += instruction->toString(newDelimiter + shift, shift, true) + "\n";
        if (breakAfter)
        {
            result += "break;\n";
        }
        result += "}\n";
    }

    if (default_)
    {
        result += getLeadingSpaces(newDelimiter) + "default:\n";
        result += default_->toString(newDelimiter + shift, shift, true) + "\n";
    }

    result += getLeadingSpaces(delimiter) + "}";

    return result;
}

void ILoopInstruction::addInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

IterLoopInstruction::IterLoopInstruction(
    const std::string &variableName, const std::string &lowerBound, const std::string &upperBound)
: variableName_(variableName)
, lowerBound_(lowerBound)
, upperBound_(upperBound)
{}

std::string IterLoopInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result = getLeadingSpaces(delimiter);
    result += "for (" + variableName_ + " = " + lowerBound_ + "; ";
    result += variableName_ + " <= " + upperBound_ + "; ";
    result += "++" + variableName_ + ")\n";
    result += getLeadingSpaces(delimiter) + "{\n";
    for (const auto &instruction : instructions_)
    {
        result += instruction->toString(shift, shift, semicolon);
    }
    result += getLeadingSpaces(delimiter) + "}";
    return result;
}

RangeLoopInstruction::RangeLoopInstruction(const std::string &variableName)
: variableName_(variableName)
{}

void RangeLoopInstruction::setRange(const std::vector<std::string> &range)
{
    range_ = range;
}

void RangeLoopInstruction::addToRange(const std::string &rangeElement)
{
    range_.push_back(rangeElement);
}

std::string RangeLoopInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result = "";
    result += getLeadingSpaces(delimiter) + "for (const auto& " + variableName_ + " : {";
    for (const auto &value : range_)
    {
        result += value + ",";
    }
    if (result.back() == ',')
    {
        result.pop_back();
    }
    result += "})\n";
    result += getLeadingSpaces(delimiter) + "{\n";
    for (const auto &instruction : instructions_)
    {
        result += instruction->toString(shift, shift, semicolon);
    }
    result += getLeadingSpaces(delimiter) + "}";
    return result;
}

BlockInstruction::BlockInstruction() {}

const std::unique_ptr<IInstruction> &BlockInstruction::frontInstruction() const
{
    return instructions_.front();
}

void BlockInstruction::pushInstructionBack(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

void BlockInstruction::pushInstructionFront(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_front(std::move(instruction));
}

void BlockInstruction::popInstructionFront()
{
    instructions_.pop_front();
}

std::string BlockInstruction::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;

    int cnt = 0;

    for (const auto &instruction : instructions_)
    {
        result += instruction->toString(delimiter, shift, true);

        cnt++;

        if (instructions_.size() > cnt)
        {
            result += "\n";
        }
    }

    return result;
}

CustomInstruction::CustomInstruction(std::string instruction)
: instruction_(instruction)
{}

std::string CustomInstruction::toString(int delimiter, int shift, bool semicolon)
{
    return addSpacesAndSemicolon(delimiter, semicolon, instruction_);
}

Function::Function(
    const std::string &name,
    const std::string &returnType,
    const std::string &attribiutes,
    bool isPublic,
    bool isConst,
    const std::string &functionNamespace)
: name_(name)
, returnType_(returnType)
, attribiutes_(attribiutes)
, isPublic_(isPublic)
, isConst_(isConst)
, functionNamespace_(functionNamespace)
{}

void Function::addArgument(std::unique_ptr<VariableDeclarationInstruction> &&var)
{
    arguments_.push_back(std::move(var));
};

void Function::addInstruction(std::unique_ptr<IInstruction> &&instruction)
{
    instructions_.push_back(std::move(instruction));
}

bool Function::isPublic()
{
    return isPublic_;
}

std::string Function::declarationToString()
{
    std::string argumentsList = getArgumentsList();
    return (attribiutes_.empty() ? attribiutes_ : attribiutes_ + " ") + returnType_ + " " + name_ + "(" +
           argumentsList + ")" + (isConst_ ? "const" : "") + ";";
}

std::string Function::getName()
{
    return name_;
}

std::string Function::getReturnType()
{
    return returnType_;
}

std::string Function::toString(int delimiter, int shift, bool semicolon)
{
    std::string result;
    std::string body;
    std::string argumentsList = getArgumentsList();

    for (auto &instruction : instructions_)
    {
        body += instruction->toString(shift, shift, true) + "\n";
    }

    result += getLeadingSpaces(delimiter) + returnType_ + " " + functionNamespace_ + name_ + "(" + argumentsList + ")" +
              (isConst_ ? "const" : "") + "\n";
    result += getLeadingSpaces(delimiter) + "{\n";
    result += body;
    result += getLeadingSpaces(delimiter) + "}\n";

    return result;
}

std::string Function::getArgumentsList()
{
    std::string argumentsList;
    for (auto &arg : arguments_)
    {
        if (arg->toString(0, 0, false) != arguments_.back()->toString(0, 0, false))
        {
            argumentsList += arg->toString(0, 0, false) + ", ";
        }
        else
        {
            argumentsList += arg->toString(0, 0, false);
        }
    }
    return argumentsList;
}

void Program::addTypeDeclaration(std::shared_ptr<IType> typeDecl)
{
    types_.push_back(std::move(typeDecl));
}

void Program::addConstantDeclaration(std::unique_ptr<IVariable> constantDecl)
{
    constants_.push_back(std::move(constantDecl));
}

void Program::addVariableDeclaration(std::unique_ptr<IVariable> variableDecl)
{
    variables_.push_back(std::move(variableDecl));
}

void Program::addFunction(std::unique_ptr<Function> &&function)
{
    functions_.push_back(std::move(function));
}

void Program::addNonGameStateFunction(std::unique_ptr<Function> &&function)
{
    nonGameStatefunctions_.push_back(std::move(function));
}

const std::vector<std::shared_ptr<IType>> &Program::getTypes() const
{
    return types_;
}

std::shared_ptr<IType> Program::findType(const std::string &identifier) const
{
    for (const auto &t : types_)
    {
        if (t->identifier == identifier)
        {
            return t;
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<IVariable>> &Program::getConstants() const
{
    return constants_;
}

const std::vector<std::unique_ptr<IVariable>> &Program::getVariables() const
{
    return variables_;
}

const std::vector<std::unique_ptr<Function>> &Program::getFunctions() const
{
    return functions_;
}

const std::vector<std::unique_ptr<Function>> &Program::getNonGameStateFunctions() const
{
    return nonGameStatefunctions_;
}

std::vector<std::string> Program::getFunctionNames(std::string retrunType) const
{
    std::vector<std::string> names;

    for (auto &func : functions_)
    {
        if (func->getReturnType() == retrunType)
        {
            names.push_back(func->getName());
        }
    }

    return names;
}
