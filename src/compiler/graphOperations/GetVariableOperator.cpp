#include <compiler/graphOperations/GetVariableOperator.hpp>

void GetVariableOperator::init()
{
    for (const auto &[edge, iid] : graph_->getAllEdges())
    {
        for (const auto &action : edge->getActions())
        {
            const auto &variable = action->getLeftSide();
            if ((action->getType() == ActionType::Assignment || action->getType() == ActionType::AssignmentAny) &&
                (variables_.find(variable) == variables_.end()))
            {
                variables_.insert(variable);
            }
        }
    }
}

const std::set<std::string> &GetVariableOperator::getVariables() const
{
    return variables_;
}

GetVariableOperator::GetVariableOperator(const std::shared_ptr<Graph> &graph)
: BaseOperator(graph)
{
    init();
}
