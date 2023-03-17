#include "BitArrayContainer.hpp"

BitArrayContainer::BitArrayContainer(
    int nodeNumber,
    const std::vector<std::pair<std::string, int>> &variablesAndDomains,
    bool isCacheOn,
    const std::string &cacheName)
: containerTypeName_("bitarray"), nodeNumber_(nodeNumber), isCacheOn_(isCacheOn), chaceName_(cacheName)
{
    generateTypeAndOrder(variablesAndDomains);
}

std::string BitArrayContainer::getType() const
{
    return keyType_;
}

std::string BitArrayContainer::getContainerDeclaration() const
{
    return containerTypeName_ + "<" + keyType_ + ">";
}

std::string BitArrayContainer::getSetMethodDeclaration(int node) const
{
    return "set" + getFunctionInput(node);
}

std::string BitArrayContainer::getIsSetMethodDeclaration(int node) const
{
    return "isSet" + getFunctionInput(node);
}

std::string BitArrayContainer::getFunctionInput(int node) const
{
    return "(" + std::to_string(node) + accessOrder_ + ")";
}

void BitArrayContainer::generateTypeAndOrder(const std::vector<std::pair<std::string, int>> &variablesAndDomains)
{
    keyType_ += "int," + std::to_string(nodeNumber_);
    for (const auto &[variable, domain] : variablesAndDomains)
    {
        keyType_ += "," + std::to_string(domain);
        accessOrder_ += "," + variable;
    }
}

ContainerType BitArrayContainer::getContainerType() const
{
    return ContainerType::BitArray;
}

std::string BitArrayContainer::getAdditionalData() const
{
    return data_;
}

const std::string BitArrayContainer::data_ =
    R"(
template< class Type, unsigned ...Ts>
class bitarray
{
public:
  void reset()
  {
    // TODO Add better reseting
    // if (++currentThreshold == 0)
    // {
    //   currentThreshold++;
    //   memset(&content_, 0, sizeof(content_));
    // }
    currentThreshold++;
  }
  template<typename P, typename I>
  bool isSet2(P &content, I i)
  {
    return content[i] == currentThreshold;
  }

  template<typename P, typename I, typename ...Is>
  bool isSet2(P &content, I i, Is ...is)
  {
    return isSet2(content[i], is...);
  }

  template<typename ...Is>
  bool isSet(Is... is)
  {
    return isSet2(content_, is...);
  }

  template<typename P, typename I>
  void set2(P &content, I i)
  {
    content[i] = currentThreshold;
  }

  template<typename P, typename I, typename ...Is>
  void set2(P &content, I i, Is ...is)
  {
    set2(content[i], is...);
  }

  template<typename ...Is>
  void set(Is... is)
  {
    return set2(content_, is...);
  }

private:

  template<class T, unsigned ... Ds> 
  struct array;

  template<class T, unsigned D > 
  struct array<T, D>
  {
    typedef T type[D];
    type data = {};
    T& operator[](unsigned i) { return data[i]; }
  };

  template<class T, unsigned D, unsigned ... Ds >
  struct array<T, D, Ds...>
  {
    typedef array<T, Ds...> BaseArray;
    typedef BaseArray type[D];
    type data = {};
    BaseArray& operator[](unsigned i) { return data[i]; }
  };

  array<Type, Ts...> content_;
  int currentThreshold = 1;
};)";