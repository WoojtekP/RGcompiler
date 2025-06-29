#pragma once

#include <map>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

using SymbolToValueMap = std::map<std::string, int>;

enum class ArithmeticSystem
{
    Overflow,
    Modular,
    Saturated,
    Comparison,
};

enum class ArithmeticOperation
{
    Inc,
    Dec,
    Add,
    Sub,
    Less,
    Leq,
    Gr,
    Ge,
};

struct ArithmeticData
{
    ArithmeticSystem system;
    ArithmeticOperation operation;
};

class IntegerOperationsDeducer
{
public:
    void fillIntegerValuesInfo(const std::vector<nlohmann::json>& integerPragmas);
    std::pair<std::string, int> getNanAndNumberOfIntegerSymbols(const SymbolToValueMap& symbolToValue) const;
    std::optional<ArithmeticData> getUnaryOperationForMap(
        const std::vector<std::string>& srcDomain,
        const std::vector<std::string>& dstDomain,
        const std::map<std::string, std::string>& constantMap) const;
    std::optional<ArithmeticData> getBinaryOperationForMap(
        const std::vector<std::string>& lhsDomain,
        const std::vector<std::string>& rhsDomain,
        const std::vector<std::string>& resultDomain,
        const std::map<std::string, std::map<std::string, std::string>>& constantMap) const;

    const SymbolToValueMap& getSymbolToValueMap() const;

private:
    std::optional<ArithmeticData> getIncOrDecWithOverflow(
        const std::vector<std::string>& srcDomain,
        const std::vector<std::string>& dstDomain,
        const std::string& nan,
        const std::map<std::string, std::string>& constantMap) const;
    std::optional<ArithmeticData> getIncOrDecWithSaturationOrModulo(
        const std::vector<std::string>& srcDomain,
        const std::vector<std::string>& dstDomain,
        const std::map<std::string, std::string>& constantMap) const;

    std::optional<std::string> getNanSymbol(const std::vector<std::string>& symbols) const;
    std::pair<std::string, std::string> getMinMaxSymbols(const std::vector<std::string>& srcDomain) const;

    SymbolToValueMap symbolToValue_;
};
