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
  }

  std::string getType(const IdType &id) const
  {
    return idToType_.at(id);
  }

  std::string getContainerDeclaration(const IdType &id) const
  {
    return containerName + "<" + idToType_.at(id) + ">";
  }

  std::string getSetDeclaration(const IdType &id, int node) const
  {
    return "insert" + getFunctionInput(id, node);
  }

  std::string getIsSetDeclaration(const IdType &id, int node) const
  {
    return "count" + getFunctionInput(id, node);
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
  std::map<std::string, int>  typeToTypeNumber_;
  std::string containerName = "std::set";

};
