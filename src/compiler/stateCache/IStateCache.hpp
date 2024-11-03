#pragma once

#include <string>


class IStateCache
{
public:
    virtual std::string getCacheType() const = 0;
    virtual std::string getCacheName() const = 0;
    virtual std::string getInsertInstruction() const = 0;
    virtual std::string getTestInstruction() const = 0;
};
