#include <vector>


class TypeDeclaration
{
    // TODO: implement!
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
    void addTypeDeclaration(TypeDeclaration typeDecl);
    void addConstantDeclaration(ConstantDeclaration constantDecl);
    void addVariableDeclaration(VariableDeclaration variableDecl);
    void addFunction(Function function);

    std::vector<TypeDeclaration> getTypes() const;
    std::vector<ConstantDeclaration> getConstants() const;
    std::vector<VariableDeclaration> getVariables() const;
    std::vector<Function> getFunctions() const;

private:
    std::vector<TypeDeclaration> types;
    std::vector<ConstantDeclaration> constants;
    std::vector<VariableDeclaration> variables;
    std::vector<Function> functions;
};
