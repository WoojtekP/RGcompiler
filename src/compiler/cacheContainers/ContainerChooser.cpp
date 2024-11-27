#include "ContainerChooser.hpp"

ContainerChooser::ContainerChooser(const std::string &cacheName)
: smallDomainMaxiumSize_(1000)
, cacheName_(cacheName)
{}

void ContainerChooser::add(const IdType &id, const VariableAndDomain &v, int nodeNumber)
{
    int domainSize = nodeNumber;

    bool isDomainSmall = (domainSize < smallDomainMaxiumSize_ ? true : false);

    if (isDomainSmall)
    {
        for (const auto &[variable, domain] : v)
        {
            if (domain == -1 || domain > smallDomainMaxiumSize_)
            {
                isDomainSmall = false;
                break;
            }

            domainSize *= domain;

            if (domainSize > smallDomainMaxiumSize_)
            {
                isDomainSmall = false;
                break;
            }
        }
    }

    std::unique_ptr<IContainer> container;
    // if (isDomainSmall)
    // {
    //     container = std::move(std::make_unique<BitArrayContainer>(nodeNumber, v, true, cacheName_));
    // }
    // else
    // {
    std::vector<std::string> variables;
    for (const auto &[name, domain] : v)
    {
        variables.push_back(name);
    }
    container = std::make_unique<UnorderedSetContainer>(variables);
    // }

    idTypeToContainer_.emplace(std::make_pair(id, std::move(container)));
    idTypeToCustomDeclaration_.insert(std::make_pair(id, getCustomName(id)));
}

const std::map<IdType, std::string> &ContainerChooser::getIdTypeToCustomDeclaration() const
{
    return idTypeToCustomDeclaration_;
}

std::string ContainerChooser::getType(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getType();
}

std::string ContainerChooser::getContainerDeclaration(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getContainerDeclaration();
}

std::string ContainerChooser::getSetMethodDeclaration(const IdType &id, int node) const
{
    return idTypeToContainer_.at(id)->getSetMethodDeclaration(node);
}

std::string ContainerChooser::getIsSetMethodDeclaration(const IdType &id, int node) const
{
    return idTypeToContainer_.at(id)->getIsSetMethodDeclaration(node);
}

std::string ContainerChooser::getAdditionalData() const
{
    std::string result = "";
    std::set<ContainerType> containerTypes;
    std::set<IdType> bitArrayContainers;

    for (const auto &[id, container] : idTypeToContainer_)
    {
        auto type = container->getContainerType();
        if (ContainerType::BitArray == type)
        {
            bitArrayContainers.insert(id);
        }
        if (containerTypes.count(type) == 0)
        {
            result += container->getAdditionalData();
        }
        containerTypes.insert(type);
    }

    result += createCache(bitArrayContainers);

    result += R"(struct vector_hash
{
  size_t operator()(const move_representation &v) const
  {
    return boost::hash_range(v.begin(), v.end());
  }
};
)";

    return result;
}

std::string ContainerChooser::getCustomName(const IdType &id) const
{
    return "container_" + std::get<0>(id) + "_" + std::get<1>(id) + "_" + std::to_string(std::get<2>(id));
}

std::string ContainerChooser::createCache(const std::set<IdType> &patterns) const
{
    std::string rgCache = "class " + cacheName_ + "{\n";
    rgCache += "public:\n";
    for (const auto &id : patterns)
    {
        rgCache += "bitarray<" + getType(id) + "> " + getCustomName(id) + ";\n";
    }
    rgCache += "};\n";

    return rgCache;
}

bool ContainerChooser::isInCache(const IdType &id) const
{
    return idTypeToContainer_.at(id)->getContainerType() == ContainerType::BitArray;
}

std::string ContainerChooser::getFromCache(const IdType &id) const
{
    return getCustomName(id);
}
