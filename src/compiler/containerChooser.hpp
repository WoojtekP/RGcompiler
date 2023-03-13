#pragma once

#include <map>
#include <string>
#include <tuple>

class ContainerChooser
{
  using IdType = std::tuple<std::string, std::string, int>;
  using SymbolMap = std::map<std::string, int>;
public:
  void add(const IdType &id, const SymbolMap &m)
  {
    auto [type, order] = generateTypeAndOrder(m);
    if (typeToTypeNumber_.count(type) == 0)
    {
      typeToTypeNumber_.insert({type, typeToTypeNumber_.size()});
    }
    idToType_[id] = type;
    typeToOrder_[type] = order;
    typeToCustomType_[type] = prefix_ + "_" + std::to_string(typeToTypeNumber_[type]);
  }

  std::string getType(const IdType &id) const
  {
    std::string type = idToType_.at(id);
    return (useCustomName ? typeToCustomType_.at(type) : type);
  }

  std::string getContainerDeclaration(const IdType &id) const
  {
    return containerName + "<" + getType(id)+ "," + "rg_hash" + ">";
  }

  std::string getSetDeclaration(const IdType &id, int node) const
  {
    return "insert" + getFunctionInput(id, node);
  }

  std::string getIsSetDeclaration(const IdType &id, int node) const
  {
    return "count" + getFunctionInput(id, node);
  }

  const std::map<std::string, std::string> &getTypeToCustomType()
  {
    return typeToCustomType_;
  }

  std::string getAdditionalData()
  {
    std::string s = R"(struct rg_hash
{
  void combine(size_t &acc, size_t x) const
  {
    acc ^= x;
  }

  template<typename T = int>
  size_t hash(int x) const
  {
    return x;
  }

  template<typename T, size_t N>
  size_t hash(std::array<T, N> a) const
  {
    size_t acc = 0;
    for (size_t i=0;i<N;i++)
    {
      combine(acc, hash(a[i]));
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
    combine(acc, hash(std::get<I>(t)));

    if constexpr(I+1 != sizeof...(Tp))
    {
      hashIter<I+1>(t, acc);
    }
  }
};)";

  return s;
  }

private:
  std::string getFunctionInput(const IdType &id, int node) const
  {
    return "({" + std::to_string(node) + typeToOrder_.at(idToType_.at(id)) + "})";
  }

  std::pair<std::string, std::string> generateTypeAndOrder(const SymbolMap &m) const
  {
      std::string idxToName[m.size()];
      for (auto p : m)
      {
          idxToName[p.second] = p.first;
      }

      std::string type = "std::tuple<int";
      std::string order;
      for (int i = 0; i < m.size(); i++)
      {
          type += ",decltype(" + idxToName[i] + ")";
          order += "," + idxToName[i];
      }

      type += ">";

      return {type, order};
  }
  std::map<IdType, SymbolMap> idToSymbolMap_;
  std::map<IdType, std::string> idToType_;
  std::map<std::string, std::string> typeToOrder_;
  std::map<std::string, std::string> typeToCustomType_;
  std::map<std::string, int>  typeToTypeNumber_;
  std::string containerName = "std::unordered_set";
  std::string prefix_ = "containerType";
  bool useCustomName = true;
};
