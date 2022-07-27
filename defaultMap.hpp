#include <map>


template<class Key, class T>
class DefaultMap : public std::map<Key, T>
{
    using value_type = std::pair<const Key, T>;

public:
    DefaultMap() = default;

    DefaultMap(const T& defaultValue, std::initializer_list<value_type> init)
    : std::map<Key, T>(std::move(init))
    , defaultValue_(defaultValue)
    {}

    DefaultMap(std::initializer_list<value_type> init)
    : DefaultMap(T(), std::move(init))
    {}

    T& operator[](const Key& key)
    {
        return std::map<Key, T>::insert(std::make_pair(key, defaultValue_)).first->second;
    }

private:
    const T defaultValue_ = T();
};
