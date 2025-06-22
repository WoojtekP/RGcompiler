#include "IntegerOperationsDeducer.hpp"

#include <set>

namespace
{
std::optional<std::string> getNanSymbolFromDomainStrict(
    const std::vector<std::string>& domain, const SymbolToValueMap& symbolToValue_)
{
    int missing = 0;
    std::string nanSymbol;
    for (const auto& symbol : domain)
    {
        if (symbolToValue_.count(symbol) == 0)
        {
            ++missing;
            nanSymbol = symbol;
        }
    }
    if (missing == 1)
    {
        return nanSymbol;
    }
    return std::nullopt;
}

bool matchesBinaryOp(
    const std::vector<std::string>& lhsDomain,
    const std::vector<std::string>& rhsDomain,
    const std::vector<std::string>& resultDomain,
    const std::map<std::string, std::map<std::string, std::string>>& constantMap,
    const SymbolToValueMap& symbolToValue_,
    std::function<int(int, int)> op,
    ArithmeticSystem system)
{
    int minVal = std::numeric_limits<int>::max();
    int maxVal = std::numeric_limits<int>::min();
    for (const auto& s : resultDomain)
    {
        if (symbolToValue_.count(s))
        {
            minVal = std::min(minVal, symbolToValue_.at(s));
            maxVal = std::max(maxVal, symbolToValue_.at(s));
        }
    }
    std::optional<std::string> nanSymbol = getNanSymbolFromDomainStrict(resultDomain, symbolToValue_);
    bool hasNan = nanSymbol.has_value();
    if (system == ArithmeticSystem::Overflow && !hasNan)
    {
        return false;
    }
    if ((system == ArithmeticSystem::Modular || system == ArithmeticSystem::Saturated) && hasNan)
    {
        return false;
    }
    for (const auto& lhs : lhsDomain)
    {
        for (const auto& rhs : rhsDomain)
        {
            auto it1 = constantMap.find(lhs);
            if (it1 == constantMap.end())
            {
                return false;
            }
            auto it2 = it1->second.find(rhs);
            if (it2 == it1->second.end())
            {
                return false;
            }
            const auto& result = it2->second;
            int lhsVal = symbolToValue_.at(lhs);
            int rhsVal = symbolToValue_.at(rhs);
            int expectedValue = op(lhsVal, rhsVal);
            switch (system)
            {
                case ArithmeticSystem::Overflow:
                    if (expectedValue < minVal || expectedValue > maxVal)
                    {
                        if (result != *nanSymbol)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        auto itSym = std::find_if(resultDomain.begin(), resultDomain.end(), [&](const std::string& s) {
                            return symbolToValue_.count(s) && symbolToValue_.at(s) == expectedValue;
                        });
                        if (itSym == resultDomain.end() || result != *itSym)
                        {
                            return false;
                        }
                    }
                    break;
                case ArithmeticSystem::Modular:
                {
                    int n = maxVal - minVal + 1;
                    int modValue = ((expectedValue - minVal) % n + n) % n + minVal;
                    auto itSym = std::find_if(resultDomain.begin(), resultDomain.end(), [&](const std::string& s) {
                        return symbolToValue_.count(s) && symbolToValue_.at(s) == modValue;
                    });
                    if (itSym == resultDomain.end() || result != *itSym)
                    {
                        return false;
                    }
                    break;
                }
                case ArithmeticSystem::Saturated:
                {
                    int satValue = std::min(std::max(expectedValue, minVal), maxVal);
                    auto itSym = std::find_if(resultDomain.begin(), resultDomain.end(), [&](const std::string& s) {
                        return symbolToValue_.count(s) && symbolToValue_.at(s) == satValue;
                    });
                    if (itSym == resultDomain.end() || result != *itSym)
                    {
                        return false;
                    }
                    break;
                }
                default:
                    return false;
            }
        }
    }
    return true;
}

bool matchesComparison(
    const std::vector<std::string>& lhsDomain,
    const std::vector<std::string>& rhsDomain,
    const std::vector<std::string>& resultDomain,
    const std::map<std::string, std::map<std::string, std::string>>& constantMap,
    const SymbolToValueMap& symbolToValue_,
    std::function<bool(int, int)> cmp)
{
    if (resultDomain.size() != 2)
    {
        return false;
    }
    const auto& falseSymbol = resultDomain[0];
    const auto& trueSymbol = resultDomain[1];
    for (const auto& lhs : lhsDomain)
    {
        for (const auto& rhs : rhsDomain)
        {
            auto it1 = constantMap.find(lhs);
            if (it1 == constantMap.end())
            {
                return false;
            }
            auto it2 = it1->second.find(rhs);
            if (it2 == it1->second.end())
            {
                return false;
            }
            const auto& result = it2->second;
            int lhsVal = symbolToValue_.at(lhs);
            int rhsVal = symbolToValue_.at(rhs);
            if (result != (cmp(lhsVal, rhsVal) ? trueSymbol : falseSymbol))
            {
                return false;
            }
        }
    }
    return true;
}
}  // namespace

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
    const std::vector<std::string>& lhsDomain,
    const std::vector<std::string>& rhsDomain,
    const std::vector<std::string>& resultDomain,
    const std::map<std::string, std::map<std::string, std::string>>& constantMap) const
{
    if (matchesComparison(lhsDomain, rhsDomain, resultDomain, constantMap, symbolToValue_, std::less<int>()))
    {
        return ArithmeticData{.system = ArithmeticSystem::Comparison, .operation = ArithmeticOperation::Less};
    }
    if (matchesComparison(lhsDomain, rhsDomain, resultDomain, constantMap, symbolToValue_, std::greater<int>()))
    {
        return ArithmeticData{.system = ArithmeticSystem::Comparison, .operation = ArithmeticOperation::Greater};
    }
    for (auto system : {ArithmeticSystem::Overflow, ArithmeticSystem::Modular, ArithmeticSystem::Saturated})
    {
        if (matchesBinaryOp(lhsDomain, rhsDomain, resultDomain, constantMap, symbolToValue_, std::plus<int>(), system))
        {
            return ArithmeticData{.system = system, .operation = ArithmeticOperation::Add};
        }
        if (matchesBinaryOp(lhsDomain, rhsDomain, resultDomain, constantMap, symbolToValue_, std::minus<int>(), system))
        {
            return ArithmeticData{.system = system, .operation = ArithmeticOperation::Sub};
        }
    }
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
