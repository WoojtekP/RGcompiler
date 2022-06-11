#pragma once

#include <memory>
#include <string>
#include <vector>


struct IType
{
    IType() = default;
    IType(const std::string id) : identifier(id) {};
    virtual ~IType() = default;
    virtual std::string toString() const = 0;

    std::string identifier;
};

struct ElementaryType : public IType
{
    ElementaryType() = default;
    ElementaryType(const std::string& id) : IType(id) {}
    ~ElementaryType() = default;
    std::string toString() const override;
};

struct FunctionType : public IType
{
    FunctionType(const std::string& id) : IType(id) {};
    FunctionType(std::unique_ptr<IType> src, std::unique_ptr<IType> dst)
    : source(std::move(src))
    , destination(std::move(dst))
    {}
    ~FunctionType() = default;
    std::string toString() const override;

    std::unique_ptr<IType> source;
    std::unique_ptr<IType> destination;
};

class ConstantDeclaration
{
    // TODO: implement!
};

class VariableDeclaration
{
    // TODO: implement!
};

class Function
{
    // TODO: implement!
};


class Program
{
public:
    void addTypeDeclaration(std::unique_ptr<IType> typeDecl);
    void addConstantDeclaration(ConstantDeclaration constantDecl);
    void addVariableDeclaration(VariableDeclaration variableDecl);
    void addFunction(Function function);

    const std::vector<std::unique_ptr<IType>>& getTypes() const;
    std::vector<ConstantDeclaration> getConstants() const;
    std::vector<VariableDeclaration> getVariables() const;
    std::vector<Function> getFunctions() const;

private:
    std::vector<std::unique_ptr<IType>> types_;
    std::vector<ConstantDeclaration> constants_;
    std::vector<VariableDeclaration> variables_;
    std::vector<Function> functions_;
};
