#include "ValueFactory.hpp"

#include <map>
#include <memory>
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

std::unique_ptr<IValue> ValueFactory::createIteratorValue(const nlohmann::json& value) const
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
                defaultValue = createListValue(entry["value"]);
            }
            else
            {
                idToValueMap.emplace(entry["identifier"].get<std::string>(), createListValue(entry["value"]));
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

std::unique_ptr<IValue> ValueFactory::createListValue(const nlohmann::json& value) const
{
    if (value["kind"] != "Map")
    {
        throw std::runtime_error("[ValueFactory] Cannot create list value for value of kind: " + value["kind"].get<std::string>());
    }
    std::vector<std::string> entries;
    for (const auto& entry : value["entries"])
    {
        if (entry["kind"] == "ValueEntry")
        {
            if (!entry["identifier"].is_null() && entry["value"]["identifier"] == "1")
            {
                entries.emplace_back(entry["identifier"].get<std::string>());
            }
        }
        else
        {
            throw std::runtime_error("[ValueFactory] Unknown type of map entry: " + entry["kind"].get<std::string>());
        }
    }
    return std::make_unique<ListValue>(std::move(entries));
}
