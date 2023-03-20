#pragma once

#include <vector>

#include "IContainer.hpp"

class StlContainerBase : public IContainer
{
public:
    StlContainerBase(std::string containerTypeName, const std::vector<std::string> &variables);
    std::string getType() const override;
    std::string getContainerDeclaration() const override;
    std::string getSetMethodDeclaration(int node) const override;
    std::string getIsSetMethodDeclaration(int node) const override;
    std::string getAdditionalData() const override;

private:
    std::string getFunctionInput(int node) const;
    void generateTypeAndOrder(const std::vector<std::string> &variables);

    std::string accessOrder_;

protected:
    std::string containerTypeName_;
    std::string keyType_;
    std::string prefix_;
};
