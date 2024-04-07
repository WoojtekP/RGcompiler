#pragma once

#include <typeindex>
#include <variant>

#include <compiler/graphOperations/GenerateGraphsOperator.hpp>
#include <compiler/graphOperations/GetEdgeOperator.hpp>
#include <compiler/graphOperations/GetOptimizedGraphOperator.hpp>
#include <compiler/graphOperations/GetTagIndexOperator.hpp>
#include <compiler/graphOperations/GetVariableOperator.hpp>
#include <compiler/graphOperations/PragmaDisjointOperator.hpp>
#include <compiler/graphOperations/PragmaSimpleApplyOperator.hpp>
#include <compiler/graphOperations/PragmaUniqueOperator.hpp>
#include <graph/Graph.hpp>

class GraphOperatorManager
{
    using OperatorKeyType = std::pair<std::type_index, Graph*>;
    using Variant = std::variant<
        std::shared_ptr<PragmaUniqueOperator>,
        std::shared_ptr<GetVariableOperator>,
        std::shared_ptr<GenerateGraphsOperator>,
        std::shared_ptr<GetEdgeOperator>,
        std::shared_ptr<GetOptimizedGraphOperator>,
        std::shared_ptr<GetTagIndexOperator>,
        std::shared_ptr<PragmaDisjointOperator>,
        std::shared_ptr<PragmaSimpleApplyOperator>>;

    std::map<OperatorKeyType, Variant> operators_;

public:
    template<class Operator>
    std::shared_ptr<Operator> getOperator(const std::shared_ptr<Graph>& graph)
    {
        OperatorKeyType key = {typeid(Operator), graph.get()};
        if (operators_.find(key) == operators_.end())
        {
            auto op = std::make_shared<Operator>(graph);
            operators_.insert({key, std::move(op)});
        }
        return std::get<std::shared_ptr<Operator>>(operators_.at(key));
    }

    template<class Operator>
    std::shared_ptr<Operator> getNewOperator(const std::shared_ptr<Graph>& graph)
    {
        return std::make_shared<Operator>(graph);
    }
};