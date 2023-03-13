#pragma once

#include "IContainer.hpp"

#include <vector>

class BitArrayContainer : public IContainer
{
public:
    BitArrayContainer(int nodeNumber, const std::vector<std::pair<std::string, int>> &variablesAndDomains);
    std::string getType() const override;
    std::string getContainerDeclaration() const override;
    std::string getSetMethodDeclaration(int node) const override;
    std::string getIsSetMethodDeclaration(int node) const override;
    std::string getAdditionalData() const override;

private:
    std::string getFunctionInput(int node) const;
    void generateTypeAndOrder(const std::vector<std::pair<std::string, int>> &variablesAndDomains);

    std::string accessOrder_;
    std::string containerTypeName_;
    std::string keyType_;
    int nodeNumber_;
};
