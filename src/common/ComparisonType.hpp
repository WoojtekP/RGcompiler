#pragma once

#include <string>

enum class ComparisonType
{
    None,
    Neg,
    Eq,
    Neq,
    Gr,
    Ge,
    Less,
    Leq,
};

std::string cmpToString(const ComparisonType cmpType);
