#include "SymbolsManager.hpp"

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
        const auto integerSymbolsCounter = operationsDeducer_.getNumberOfIntegerSymbols(symbolToValue);
        if (integerSymbolsCounter == symbolToValue.size())
        {
            integerTypes_.withoutNan.insert(typeName);
        }
        if (integerSymbolsCounter == symbolToValue.size() - 1)
        {
            integerTypes_.withNan.insert(typeName);
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
        const auto dstType = parser_.getDestinationType(type);
        if (parser_.isArrayType(dstType))
        {
            // TODO: implement
        }
        else
        {
            const auto dstTypeId = dstType["identifier"].get<std::string>();
            if (integerTypes_.isWithoutNan(srcTypeId) && integerTypes_.isAnyInt(dstTypeId))
            {
                const auto constantMap = getUnaryMapFromConstant(srcTypeId, dstType, constant);
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
    const std::string& srcTypeId, const nlohmann::json& dstType, const nlohmann::json& constant)
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
