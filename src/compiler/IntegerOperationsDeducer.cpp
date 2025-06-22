#include "IntegerOperationsDeducer.hpp"

#include <set>

void IntegerOperationsDeducer::fillIntegerValuesInfo(const std::vector<nlohmann::json>& integerPragmas)
{
    for (const auto& integerPragma : integerPragmas)
    {
        auto value = integerPragma["offset"].get<int>();
        for (const auto& item : integerPragma["edgeNames"])
        {
            const auto symbol = item["identifier"].get<std::string>();
            symbolToValue_.emplace(symbol, value++);
        }
    }
}

std::pair<std::string, int> IntegerOperationsDeducer::getNanAndNumberOfIntegerSymbols(const SymbolToValueMap& symbolToValue) const
{
    int integerSymbolsCounter = 0;
    std::string nanSymbol;
    for (const auto& [symbol, _] : symbolToValue)
    {
        if (symbolToValue_.count(symbol))
        {
            ++integerSymbolsCounter;
        }
        else
        {
            nanSymbol = symbol;
        }
    }
    return std::make_pair(nanSymbol, integerSymbolsCounter);
}

const SymbolToValueMap& IntegerOperationsDeducer::getSymbolToValueMap() const
{
    return symbolToValue_;
}

std::optional<ArithmeticData> IntegerOperationsDeducer::getUnaryOperationForMap(
    const std::vector<std::string>& srcDomain,
    const std::vector<std::string>& dstDomain,
    const std::map<std::string, std::string>& constantMap) const
{
    const auto nan = getNanSymbol(dstDomain);
    if (nan.has_value())
    {
        return getIncOrDecWithOverflow(srcDomain, dstDomain, *nan, constantMap);
    }
    return getIncOrDecWithSaturationOrModulo(srcDomain, dstDomain, constantMap);
}

std::optional<ArithmeticData> IntegerOperationsDeducer::getBinaryOperationForMap(
    const std::map<std::string, std::map<std::string, std::string>>& constantMap) const
{
    // TODO: implement
    return std::nullopt;
}

std::optional<ArithmeticData> IntegerOperationsDeducer::getIncOrDecWithOverflow(
    const std::vector<std::string>& srcDomain,
    const std::vector<std::string>& dstDomain,
    const std::string& nan,
    const std::map<std::string, std::string>& constantMap) const
{
    const auto [minSymbol, maxSymbol] = getMinMaxSymbols(srcDomain);
    std::set<int> differences;
    for (const auto& srcSymbol : srcDomain)
    {
        const auto dstSymbol = constantMap.at(srcSymbol);
        if (dstSymbol == nan)
        {
            if (srcSymbol == maxSymbol)
            {
                differences.insert(1);
            }
            else if (srcSymbol == minSymbol)
            {
                differences.insert(-1);
            }
            else
            {
                return std::nullopt;
            }
        }
        else
        {
            const auto& srcValue = symbolToValue_.at(srcSymbol);
            const auto& dstValue = symbolToValue_.at(dstSymbol);
            differences.insert(dstValue - srcValue);
        }
    }
    if (differences == std::set{-1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Overflow, .operation = ArithmeticOperation::Dec};
    }
    if (differences == std::set{1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Overflow, .operation = ArithmeticOperation::Inc};
    }
    return std::nullopt;
}

std::optional<ArithmeticData> IntegerOperationsDeducer::getIncOrDecWithSaturationOrModulo(
    const std::vector<std::string>& srcDomain,
    const std::vector<std::string>& dstDomain,
    const std::map<std::string, std::string>& constantMap) const
{
    const auto [minSymbol, maxSymbol] = getMinMaxSymbols(srcDomain);
    std::set<int> differences;
    for (const auto& srcSymbol : srcDomain)
    {
        const auto dstSymbol = constantMap.at(srcSymbol);
        if ((srcSymbol == maxSymbol && dstSymbol == maxSymbol) || (srcSymbol == minSymbol && dstSymbol == minSymbol))
        {
            differences.insert(0);
        }
        else if (srcSymbol == maxSymbol && dstSymbol == minSymbol)
        {
            differences.insert(1);
        }
        else if (srcSymbol == minSymbol && dstSymbol == maxSymbol)
        {
            differences.insert(-1);
        }
        else
        {
            const auto& srcValue = symbolToValue_.at(srcSymbol);
            const auto& dstValue = symbolToValue_.at(dstSymbol);
            differences.insert(dstValue - srcValue);
        }
    }
    if (differences == std::set{0, 1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Saturated, .operation = ArithmeticOperation::Inc};
    }
    if (differences == std::set{0, -1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Saturated, .operation = ArithmeticOperation::Dec};
    }
    if (differences == std::set{-1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Modular, .operation = ArithmeticOperation::Dec};
    }
    if (differences == std::set{1})
    {
        return ArithmeticData{.system = ArithmeticSystem::Modular, .operation = ArithmeticOperation::Inc};
    }
    return std::nullopt;
}

std::optional<std::string> IntegerOperationsDeducer::getNanSymbol(const std::vector<std::string>& symbols) const
{
    for (const auto& symbol : symbols)
    {
        if (symbolToValue_.count(symbol) == 0)
        {
            return symbol;
        }
    }
    return std::nullopt;
}

std::pair<std::string, std::string> IntegerOperationsDeducer::getMinMaxSymbols(
    const std::vector<std::string>& srcDomain) const
{
    const auto [minValueIt, maxValueIt] =
        std::minmax_element(srcDomain.begin(), srcDomain.end(), [this](const auto& lhs, const auto& rhs) {
            return symbolToValue_.at(lhs) < symbolToValue_.at(rhs);
        });
    return std::make_pair(*minValueIt, *maxValueIt);
}
