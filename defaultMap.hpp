#pragma once

#include <map>


template<class Key, class T>
class DefaultMap : public std::map<Key, T>
{
public:
    using value_type = std::pair<const Key, T>;

    DefaultMap() = default;

    DefaultMap(const T& defaultValue, std::initializer_list<value_type> init)
    : std::map<Key, T>(std::move(init))
    , defaultValue_(defaultValue)
    {}

    T& operator[](const Key& key)
    {
        return std::map<Key, T>::insert(std::make_pair(key, defaultValue_)).first->second;
    }

    const T& operator[](const Key& key) const
    {
        const auto it = std::map<Key, T>::find(key);
        return it == std::map<Key, T>::end() ? defaultValue_ : it->second;
    }

private:
    const T defaultValue_ = T();
};
