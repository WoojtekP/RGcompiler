#pragma once

#include <nlohmann/json.hpp>

#include <compiler/IntegerOperationsDeducer.hpp>
#include <compiler/ValueAssigner.hpp>
#include <parser/Parser.hpp>

using ConstantToOperation = std::map<std::string, ArithmeticData>;

class SymbolsManager
{
public:
    SymbolsManager(const Parser& parser);
    const ValueAssigner& getValueAssigner() const;
    const ConstantToOperation& constantToArithmeticOperationMap() const;
    std::pair<std::string, std::string> getMinMaxArithmeticSymbols(
        const std::vector<std::string>& symbols) const;
    bool isNan(const std::string& symbol) const;

    void fillIntegerOperationsData();
    void assignValuesForSymbolsAndTags();

private:
    std::map<std::string, std::string> getUnaryMapFromConstant(
        const std::string& srcType, const nlohmann::json& constant);
    std::map<std::string, std::map<std::string, std::string>> getBinaryMapFromConstant(
        const std::string& srcTypeId, const std::string& srcSndTypeId, const nlohmann::json& constant);

    struct IntegerTypes
    {
        std::set<std::string> withoutNan;
        std::set<std::string> withNan;
        std::set<std::string> nanSymbols;
        bool isAnyInt(const std::string& type) const;
        bool isWithoutNan(const std::string& type) const;
        bool isWithNan(const std::string& type) const;
    };

    const Parser& parser_;
    ValueAssigner valueAssigner_;
    IntegerOperationsDeducer operationsDeducer_;
    IntegerTypes integerTypes_;
    std::map<std::string, ArithmeticData> constantToArithmeticOperation_;
};
