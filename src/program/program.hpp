#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

struct IType
{
    IType() = default;
    IType(const std::string &id) : identifier(id) {};
    virtual ~IType() = default;
    virtual std::string toString() const = 0;
    virtual std::string definitionToString() const = 0;

    std::string identifier;
};

struct ElementaryType : public IType
{
    ElementaryType() = default;
    ElementaryType(const std::string &id) : IType(id) {}
    ~ElementaryType() = default;
    std::string toString() const override;
    std::string definitionToString() const override;
};

struct FunctionType : public IType
{
    FunctionType(const std::string &id) : IType(id) {};
    FunctionType(std::shared_ptr<IType> src, std::shared_ptr<IType> dst)
    : source(std::move(src)), destination(std::move(dst))
    {}
    ~FunctionType() = default;
    std::string toString() const override;
    std::string definitionToString() const override;

    std::shared_ptr<IType> source;
    std::shared_ptr<IType> destination;
};

struct CustomType : public IType
{
    CustomType() = default;
    CustomType(const std::string &id) : IType(id) {}
    CustomType(const std::string &id, const std::string &typeDef) : IType(id), typeDefinition(typeDef) {}
    ~CustomType() = default;
    std::string toString() const override;
    std::string definitionToString() const override;

    std::string typeDefinition;
};

struct IValue
{
    IValue() = default;
    virtual ~IValue() = default;
    virtual std::string toString() const = 0;
};

struct SingleValue : public IValue
{
    SingleValue() = default;
    SingleValue(const std::string &sym) : symbol(sym) {}
    ~SingleValue() = default;
    std::string toString() const override;

    std::string symbol;
};

struct MapValue : public IValue
{
    MapValue() = default;
    MapValue(std::map<std::string, std::unique_ptr<IValue>> idToValue, std::unique_ptr<IValue> defaultVal)
    : idToValueMap(std::move(idToValue)), defaultValue(std::move(defaultVal))
    {}
    ~MapValue() = default;
    std::string toString() const override;

    std::map<std::string, std::unique_ptr<IValue>> idToValueMap;
    std::unique_ptr<IValue> defaultValue;
};

struct IVariable
{
    IVariable() = default;
    IVariable(const std::string &id, std::shared_ptr<IType> valType, bool isPublic = false)
    : identifier(id), valueType(std::move(valType)), isPublic_(isPublic)
    {}
    IVariable(const std::string &id, std::shared_ptr<IType> valType, std::unique_ptr<IValue> val, bool isPublic = false)
    : identifier(id), valueType(std::move(valType)), value(std::move(val)), isPublic_(isPublic)
    {}
    virtual ~IVariable() = default;
    virtual std::string toString() const = 0;
    bool isPublic() { return isPublic_; }

    bool isPublic_;
    std::string identifier;
    std::shared_ptr<IType> valueType;
    std::unique_ptr<IValue> value;
};

struct Constant : public IVariable
{
    Constant() = default;
    Constant(const std::string &id, std::shared_ptr<IType> valType, std::unique_ptr<IValue> val)
    : IVariable(id, std::move(valType), std::move(val))
    {}
    ~Constant() = default;
    std::string toString() const override;
};

struct Variable : public IVariable
{
    Variable() = default;
    Variable(const std::string &id, std::shared_ptr<IType> valType, bool isPublic = false)
    : IVariable(id, std::move(valType), isPublic)
    {}
    Variable(const std::string &id, std::shared_ptr<IType> valType, std::unique_ptr<IValue> val, bool isPublic = false)
    : IVariable(id, std::move(valType), std::move(val), isPublic)
    {}
    ~Variable() = default;
    std::string toString() const override;
};

class IInstruction
{
protected:
    inline std::string getLeadingSpaces(int n) { return std::string(n, ' '); }

    inline std::string getSemicolon(bool n) { return (n ? ";" : ""); }

    inline std::string addSpacesAndSemicolon(int delimiter, bool semicolon, const std::string &str)
    {
        return getLeadingSpaces(delimiter) + str + getSemicolon(semicolon);
    }

public:
    virtual ~IInstruction() = default;
    virtual std::string toString(int delimiter, int shift, bool semicolon) = 0;
};

class ReturnInstruction : public IInstruction
{
    std::string value_;

public:
    ReturnInstruction();
    ReturnInstruction(const std::string &value);

    std::string toString(int delimiter, int shift, bool semicolon);
};

class VariableDeclarationInstruction : public IInstruction
{
    std::string name_;
    std::string type_;

public:
    VariableDeclarationInstruction(const std::string &name);
    VariableDeclarationInstruction(const std::string &name, std::string type);

    std::string toString(int delimiter, int shift, bool semicolon);
};

class AssignmentInstruction : public IInstruction
{
    std::unique_ptr<VariableDeclarationInstruction> left_;
    std::string right_;

public:
    AssignmentInstruction(const std::string &left, const std::string &right);
    AssignmentInstruction(const std::string &left, const std::string &right, std::string type);

    std::string toString(int delimiter, int shift, bool semicolon);
};

class ComparisonInstruction : public IInstruction
{
    std::string left_;
    std::string right_;
    bool negated_;

public:
    ComparisonInstruction(const std::string &left, const std::string &right);
    ComparisonInstruction(const std::string &left, const std::string &right, bool negated);

    std::string toString(int delimiter, int shift, bool semicolon) override;
};

class BlockInstruction : public IInstruction
{
    std::vector<std::unique_ptr<IInstruction>> instructions_;

public:
    BlockInstruction();

    void addInstruction(std::unique_ptr<IInstruction> &&instruction);

    std::string toString(int delimiter, int shift, bool semicolon) override;
};

class IfInstruction : public IInstruction
{
    std::unique_ptr<ComparisonInstruction> condition_;
    std::vector<std::unique_ptr<IInstruction>> instructions_;

public:
    IfInstruction(std::unique_ptr<ComparisonInstruction> &&condition);

    void addInstruction(std::unique_ptr<IInstruction> &&instruction);

    std::string toString(int delimiter, int shift, bool semicolon) override;
};

class SwitchInstruction : public IInstruction
{
    std::string condition_;
    std::vector<std::pair<int, std::unique_ptr<IInstruction>>> instructions_;
    std::unique_ptr<IInstruction> default_;

public:
    SwitchInstruction(const std::string &condition);

    void addCaseInstruction(int val, std::unique_ptr<IInstruction> &&instruction);
    void addDefaultInstruction(std::unique_ptr<IInstruction> &&instruction);

    std::string toString(int delimiter, int shift, bool semicolon) override;
};

class LoopInstruction : public IInstruction
{
    // TODO: implement!
};

class CustomInstruction : public IInstruction
{
    std::string instruction_;

public:
    CustomInstruction(std::string instruction);

    std::string toString(int delimiter, int shift, bool semicolon) override;
};

class Function : public IInstruction
{
    bool isPublic_;
    std::string name_;
    std::string returnType_;
    std::vector<std::unique_ptr<IInstruction>> instructions_;
    std::vector<std::unique_ptr<VariableDeclarationInstruction>> arguments_;

public:
    Function(std::string name, std::string returnType, bool isPublic = false);

    void addArgument(std::unique_ptr<VariableDeclarationInstruction> &&var);
    void addInstruction(std::unique_ptr<IInstruction> &&instruction);

    bool isPublic();
    std::string declarationToString();
    std::string toString(int delimiter, int shift, bool semicolon) override;
    std::string getName();
    std::string getReturnType();

private:
    std::string getArgumentsList();
};

class Program
{
public:
    void addTypeDeclaration(std::shared_ptr<IType> typeDecl);
    void addConstantDeclaration(std::unique_ptr<IVariable> constantDecl);
    void addVariableDeclaration(std::unique_ptr<IVariable> variableDecl);
    void addFunction(std::unique_ptr<Function> &&function);

    const std::vector<std::shared_ptr<IType>> &getTypes() const;
    const std::vector<std::unique_ptr<IVariable>> &getConstants() const;
    const std::vector<std::unique_ptr<IVariable>> &getVariables() const;
    const std::vector<std::unique_ptr<Function>> &getFunctions() const;
    std::vector<std::string> getFunctionNames(std::string returnType) const;

private:
    std::vector<std::shared_ptr<IType>> types_;
    std::vector<std::unique_ptr<IVariable>> constants_;
    std::vector<std::unique_ptr<IVariable>> variables_;
    std::vector<std::unique_ptr<Function>> functions_;
};
