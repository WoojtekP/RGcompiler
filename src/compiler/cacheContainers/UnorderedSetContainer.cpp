#include "UnorderedSetContainer.hpp"

UnorderedSetContainer::UnorderedSetContainer(const std::vector<std::string> &variables)
: StlContainerBase("unordered_set", variables)
{}

std::string UnorderedSetContainer::getContainerDeclaration() const
{
    return prefix_ + containerTypeName_ + "<" + keyType_ + "," + "rg_hash" + ">";
}

ContainerType UnorderedSetContainer::getContainerType() const
{
    return ContainerType::UnorderedSet;
}

std::string UnorderedSetContainer::getAdditionalData() const
{
    return data_;
}

const std::string UnorderedSetContainer::data_ =
    R"(
struct rg_hash
{
  template<typename T, size_t N>
  size_t hash(std::array<T, N> a) const
  {
    size_t acc = 0;
    for (size_t i=0;i<N;i++)
    {
      boost::hash_combine(acc, a[i]);
    }

    return acc;
  }

  template<typename ...Tp>
  size_t operator()(const std::tuple<Tp...> &t) const
  {
    size_t acc = 0;
    hashIter<0 ,Tp...>(t, acc);
    return acc;
  }

  template<size_t I = 0, typename... Tp>
  void hashIter(const std::tuple<Tp...>& t, size_t &acc) const
  {
    boost::hash_combine(acc, std::get<I>(t));

    if constexpr(I+1 != sizeof...(Tp))
    {
      hashIter<I+1>(t, acc);
    }
  }
};)";
