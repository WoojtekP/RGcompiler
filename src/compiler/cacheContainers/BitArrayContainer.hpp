#pragma once

#include <vector>

#include "IContainer.hpp"

class BitArrayContainer : public IContainer
{
public:
    BitArrayContainer(
        int nodeNumber,
        const std::vector<std::pair<std::string, int>> &variablesAndDomains,
        bool isCacheOn = false,
        const std::string &cacheName = "");
    ContainerType getContainerType() const override;
    std::string getType() const override;
    std::string getContainerDeclaration() const override;
    std::string getSetMethodDeclaration(int node) const override;
    std::string getIsSetMethodDeclaration(int node) const override;
    std::string getAdditionalData() const override;

private:
    std::string getFunctionInput(int node) const;
    void generateTypeAndOrder(const std::vector<std::pair<std::string, int>> &variablesAndDomains);

    static const std::string data_;
    std::string accessOrder_;
    std::string containerTypeName_;
    std::string keyType_;
    const std::string chaceName_;
    const bool isCacheOn_;
    int nodeNumber_;
};
