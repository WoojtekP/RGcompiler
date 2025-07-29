#include "ValueFactory.hpp"

#include <map>
#include <memory>
#include <ranges>
#include <string>

#include <program/Program.hpp>

std::unique_ptr<IValue> ValueFactory::createValue(const nlohmann::json& value) const
{
    if (value["kind"] == "Element")
    {
        return std::make_unique<SingleValue>(value["identifier"].get<std::string>());
    }
    else if (value["kind"] == "Map")
    {
        return createMapValue(value);
    }
    throw std::runtime_error("[ValueFactory] Unknown kind of value " + value["kind"].get<std::string>());
}

std::unique_ptr<IValue> ValueFactory::createIteratorValue(
    const nlohmann::json& value, const std::vector<std::string>& iteratorDomain) const
{
    if (value["kind"] != "Map")
    {
        throw std::runtime_error("[ValueFactory] Cannot create iterator value for value of kind: " + value["kind"].get<std::string>());
    }
    std::map<std::string, std::unique_ptr<IValue>> idToValueMap;
    std::unique_ptr<IValue> defaultValue;
    for (const auto& entry : value["entries"])
    {
        if (entry["kind"] == "ValueEntry")
        {
            if (entry["identifier"].is_null())
            {
                defaultValue = createListValue(entry["value"], iteratorDomain);
            }
            else
            {
                idToValueMap.emplace(entry["identifier"].get<std::string>(), createListValue(entry["value"], iteratorDomain));
            }
        }
        else
        {
            throw std::runtime_error("[ValueFactory] Unknown type of map entry: " + entry["kind"].get<std::string>());
        }
    }
    return std::make_unique<MapValue>(std::move(idToValueMap), std::move(defaultValue));
}

std::unique_ptr<IValue> ValueFactory::createMapValue(const nlohmann::json& value) const
{
    std::map<std::string, std::unique_ptr<IValue>> idToValueMap;
    std::unique_ptr<IValue> defaultValue;
    for (const auto& entry : value["entries"])
    {
        if (entry["kind"] == "ValueEntry")
        {
            if (entry["identifier"].is_null())
            {
                defaultValue = createValue(entry["value"]);
            }
            else
            {
                idToValueMap.emplace(entry["identifier"].get<std::string>(), createValue(entry["value"]));
            }
        }
        else
        {
            throw std::runtime_error("[ValueFactory] Unknown type of map entry: " + entry["kind"].get<std::string>());
        }
    }
    return std::make_unique<MapValue>(std::move(idToValueMap), std::move(defaultValue));
}

std::unique_ptr<IValue> ValueFactory::createListValue(const nlohmann::json& value, const std::vector<std::string>& iteratorDomain) const
{
    if (value["kind"] != "Map")
    {
        throw std::runtime_error("[ValueFactory] Cannot create list value for value of kind: " + value["kind"].get<std::string>());
    }
    std::vector<std::string> trueEntries, falseEntries;
    bool isDefaultValueTrue = false;
    for (const auto& entry : value["entries"])
    {
        if (entry["kind"] == "ValueEntry")
        {
            if (entry["identifier"].is_null())
            {
                isDefaultValueTrue = (entry["value"]["identifier"] == "1");
            }
            else if (entry["value"]["identifier"] == "1")
            {
                trueEntries.emplace_back(entry["identifier"].get<std::string>());
            }
            else if (entry["value"]["identifier"] == "0")
            {
                falseEntries.emplace_back(entry["identifier"].get<std::string>());
            }
            else
            {
                throw std::runtime_error("[ValueFactory] Unhandled value of entry: " + entry["identifier"].get<std::string>());
            }
        }
        else
        {
            throw std::runtime_error("[ValueFactory] Unknown type of map entry: " + entry["kind"].get<std::string>());
        }
    }
    if (isDefaultValueTrue)
    {
        std::vector<std::string> finalEntries;
        std::ranges::copy_if(
            iteratorDomain,
            std::back_inserter(finalEntries),
            [&falseEntries](const std::string& symbol) {
                return std::find(falseEntries.begin(), falseEntries.end(), symbol) == falseEntries.end();
            });
        return std::make_unique<ListValue>(std::move(finalEntries));
    }
    else
    {
        return std::make_unique<ListValue>(std::move(trueEntries));
    }
}
