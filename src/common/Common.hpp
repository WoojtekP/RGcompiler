#pragma once
#include <memory>
#include <string_view>

#include <graph/Action.hpp>
namespace common
{
constexpr std::string_view playerWord = "player";
constexpr std::string_view keeperWord = "keeper";
constexpr std::string_view keeperCastedWord = "static_cast<PlayerOrSystem>(keeper)";

inline bool isActionAssignmentToPlayer(const std::shared_ptr<IAction>& action)
{
    return action->getType() == ActionType::Assignment && action->getLeftSide() == playerWord;
}

inline bool isActionAssignmentKeeperToPlayer(const std::shared_ptr<IAction>& action)
{
    return action->getType() == ActionType::Assignment && action->getLeftSide() == playerWord &&
           (action->getRightSide() == keeperCastedWord || action->getRightSide() == keeperWord);
}

}  // namespace common
