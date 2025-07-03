#include "SymbolsManager.hpp"

#include <limits.h>

#include <compiler/ValueFactory.hpp>
#include <parser/Parser.hpp>
#include <program/Program.hpp>

SymbolsManager::SymbolsManager(const Parser& parser)
: parser_(parser)
{}

const ValueAssigner& SymbolsManager::getValueAssigner() const
{
    return valueAssigner_;
}

const ConstantToOperation& SymbolsManager::constantToArithmeticOperationMap() const
{
    return constantToArithmeticOperation_;
}

bool SymbolsManager::isNan(const std::string& symbol) const
{
    return integerTypes_.nanSymbols.contains(symbol);
}

std::pair<std::string, std::string> SymbolsManager::getMinMaxArithmeticSymbols(
    const std::vector<std::string>& symbols) const
{
    const auto& symbolToValueMap = operationsDeducer_.getSymbolToValueMap();
    std::string minSymbol, maxSymbol;
    int minValue = INT_MAX, maxValue = INT_MIN;
    for (const auto& symbol : symbols)
    {
        const auto symbolToValueIt = symbolToValueMap.find(symbol);
        if (symbolToValueIt != symbolToValueMap.end())
        {
            if (symbolToValueIt->second > maxValue)
            {
                maxSymbol = symbolToValueIt->first;
            }
            else if (symbolToValueIt->second < minValue)
            {
                minSymbol = symbolToValueIt->first;
            }
        }
    }
    return std::make_pair(minSymbol, maxSymbol);
}

void SymbolsManager::assignValuesForSymbolsAndTags()
{
    valueAssigner_.assignValuesForSymbols(operationsDeducer_.getSymbolToValueMap(), parser_.getTypeDeclarations());
    valueAssigner_.assignValuesForTags(parser_, parser_.getEdges());
}

void SymbolsManager::fillIntegerOperationsData()
{
    operationsDeducer_.fillIntegerValuesInfo(parser_.getPragmas("Integer"));
    assignValuesForSymbolsAndTags();

    integerTypes_.withNan.clear();
    integerTypes_.withoutNan.clear();

    for (const auto& [typeName, symbolToValue] : valueAssigner_.getTypeToSymbolToValueMap())
    {
        const auto [nanSymbol, integerSymbolsCounter] =
            operationsDeducer_.getNanAndNumberOfIntegerSymbols(symbolToValue);
        if (integerSymbolsCounter == symbolToValue.size())
        {
            integerTypes_.withoutNan.insert(typeName);
        }
        if (integerSymbolsCounter == symbolToValue.size() - 1)
        {
            integerTypes_.withNan.insert(typeName);
            integerTypes_.nanSymbols.insert(nanSymbol);
        }
    }

    for (const auto& constant : parser_.getConstants())
    {
        const auto type = constant["type"];
        if (!parser_.isArrayType(type))
        {
            continue;
        }
        const auto srcTypeId = parser_.getSourceType(type);
        auto dstType = parser_.getDestinationType(type);
        if (parser_.isArrayType(dstType))
        {
            const auto srcSndTypeId = parser_.getSourceType(dstType);
            dstType = parser_.getDestinationType(dstType);
            if (parser_.isArrayType(dstType) || srcTypeId != srcSndTypeId)
            {
                continue;
            }
            const auto dstTypeId = dstType["identifier"].get<std::string>();
            // TODO: fix - do not use "Bool" constant
            if (integerTypes_.isWithoutNan(srcTypeId) && integerTypes_.isWithoutNan(srcSndTypeId) &&
                (integerTypes_.isAnyInt(dstTypeId) || dstTypeId == "Bool"))
            {
                const auto constantMap = getBinaryMapFromConstant(srcTypeId, srcSndTypeId, constant);
                const auto& srcTypeDomain = parser_.getDomain(srcTypeId);
                const auto& srcSndTypeDomain = parser_.getDomain(srcSndTypeId);
                const auto& dstTypeDomain = parser_.getDomain(dstTypeId);
                if (const auto operation = operationsDeducer_.getBinaryOperationForMap(
                        srcTypeDomain, srcSndTypeDomain, dstTypeDomain, constantMap))
                {
                    constantToArithmeticOperation_.emplace(constant["identifier"].get<std::string>(), *operation);
                }
            }
        }
        else
        {
            const auto dstTypeId = dstType["identifier"].get<std::string>();
            if (integerTypes_.isWithoutNan(srcTypeId) && integerTypes_.isAnyInt(dstTypeId))
            {
                const auto constantMap = getUnaryMapFromConstant(srcTypeId, constant);
                const auto& srcTypeDomain = parser_.getDomain(srcTypeId);
                const auto& dstTypeDomain = parser_.getDomain(dstTypeId);
                if (const auto operation =
                        operationsDeducer_.getUnaryOperationForMap(srcTypeDomain, dstTypeDomain, constantMap))
                {
                    constantToArithmeticOperation_.emplace(constant["identifier"].get<std::string>(), *operation);
                }
            }
        }
    }
}

std::map<std::string, std::string> SymbolsManager::getUnaryMapFromConstant(
    const std::string& srcTypeId, const nlohmann::json& constant)
{
    ValueFactory valueFactory;
    const auto constValue = valueFactory.createValue(constant["value"]);

    MapValue* mapValue = dynamic_cast<MapValue*>(constValue.get());
    assert(mapValue != nullptr);
    const auto& idToValueMap = mapValue->idToValueMap;
    SingleValue* defaultValue = dynamic_cast<SingleValue*>(mapValue->defaultValue.get());
    assert(defaultValue != nullptr);

    std::map<std::string, std::string> map;
    for (const auto& symbol : parser_.getDomain(srcTypeId))
    {
        const auto valueIt = idToValueMap.find(symbol);
        if (valueIt != idToValueMap.end())
        {
            const auto singleValue = dynamic_cast<SingleValue*>(valueIt->second.get());
            map.emplace(symbol, singleValue->symbol);
        }
        else
        {
            map.emplace(symbol, defaultValue->symbol);
        }
    }
    return map;
}

std::map<std::string, std::map<std::string, std::string>> SymbolsManager::getBinaryMapFromConstant(
    const std::string& srcTypeId, const std::string& srcSndTypeId, const nlohmann::json& constant)
{
    ValueFactory valueFactory;
    const auto constValue = valueFactory.createValue(constant["value"]);
    MapValue* outerMap = dynamic_cast<MapValue*>(constValue.get());
    assert(outerMap != nullptr);
    const auto& outerIdToValueMap = outerMap->idToValueMap;
    SingleValue* outerDefault = dynamic_cast<SingleValue*>(outerMap->defaultValue.get());
    assert(outerDefault != nullptr || dynamic_cast<MapValue*>(outerMap->defaultValue.get()) != nullptr);

    std::map<std::string, std::map<std::string, std::string>> map;
    const auto& domain1 = parser_.getDomain(srcTypeId);
    const auto& domain2 = parser_.getDomain(srcSndTypeId);
    for (const auto& symbol1 : domain1)
    {
        MapValue* innerMap = nullptr;
        SingleValue* innerDefault = nullptr;
        auto outerIt = outerIdToValueMap.find(symbol1);
        if (outerIt != outerIdToValueMap.end())
        {
            innerMap = dynamic_cast<MapValue*>(outerIt->second.get());
            assert(innerMap != nullptr);
            innerDefault = dynamic_cast<SingleValue*>(innerMap->defaultValue.get());
            assert(innerDefault != nullptr);
        }
        else
        {
            innerMap = dynamic_cast<MapValue*>(outerMap->defaultValue.get());
            assert(innerMap != nullptr);
            innerDefault = dynamic_cast<SingleValue*>(innerMap->defaultValue.get());
            assert(innerDefault != nullptr);
        }
        for (const auto& symbol2 : domain2)
        {
            auto innerIt = innerMap->idToValueMap.find(symbol2);
            if (innerIt != innerMap->idToValueMap.end())
            {
                auto singleValue = dynamic_cast<SingleValue*>(innerIt->second.get());
                assert(singleValue != nullptr);
                map[symbol1][symbol2] = singleValue->symbol;
            }
            else
            {
                map[symbol1][symbol2] = innerDefault->symbol;
            }
        }
    }
    return map;
}

bool SymbolsManager::IntegerTypes::isAnyInt(const std::string& type) const
{
    return isWithoutNan(type) || isWithNan(type);
}

bool SymbolsManager::IntegerTypes::isWithoutNan(const std::string& type) const
{
    return withoutNan.count(type);
}

bool SymbolsManager::IntegerTypes::isWithNan(const std::string& type) const
{
    return withNan.count(type);
}
