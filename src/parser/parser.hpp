#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include <nlohmann/json.hpp>


class Parser
{
public:
    Parser(std::ifstream& jsonGameFile);
    bool isSymbol(const std::string& token) const;
    bool isConstant(const std::string& token) const;
    bool isVariable(const std::string& token) const;
    nlohmann::json getTypeDeclarations() const;
    nlohmann::json getVariables() const;
    nlohmann::json getConstants() const;
    nlohmann::json getEdges() const;
    std::vector<std::string> getDomain(const std::string& typeIdentifier) const;
    std::string getSourceType(const nlohmann::json& t) const;
    nlohmann::json getDestinationType(const nlohmann::json& t) const;
    nlohmann::json findTypeOfExpression(const nlohmann::json& expression) const;
    nlohmann::json findTypeOfVariable(const std::string& identifier) const;
    nlohmann::json findTypeByIdentifier(const std::string& typeIdentifier) const;

    static std::string getValueFromEntries(
        const nlohmann::json& entries, const std::string& entryKind, const std::string& entryName);
    static std::optional<std::reference_wrapper<const nlohmann::json>> getPartFromParts(
        const nlohmann::json& parts, const std::string& entryKind);

private:
    nlohmann::json parsedJson_;
    std::set<std::string> symbols_;
    std::set<std::string> constants_;
    std::set<std::string> variables_;

};
