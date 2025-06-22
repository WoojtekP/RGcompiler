#include "ComparisonType.hpp"

#include <string>
#include <stdexcept>

std::string cmpToString(const ComparisonType cmpType)
{
    switch (cmpType)
    {
        case ComparisonType::None:
            return "";
        case ComparisonType::Neg:
            return "!";
        case ComparisonType::Eq:
            return "==";
        case ComparisonType::Neq:
            return "!=";
        case ComparisonType::Gr:
            return ">";
        case ComparisonType::Ge:
            return ">=";
        case ComparisonType::Less:
            return "<";
        case ComparisonType::Leq:
            return "<=";
    }
    throw std::invalid_argument("Unkwnon ComparisonType: " + std::to_string(static_cast<int>(cmpType)));
}
