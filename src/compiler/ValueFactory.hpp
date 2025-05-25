#pragma once

#include <memory>

#include <nlohmann/json.hpp>

struct IValue;

class ValueFactory
{
public:
    std::unique_ptr<IValue> createValue(const nlohmann::json& value) const;

private:
    std::unique_ptr<IValue> createMapValue(const nlohmann::json& value) const;
};
