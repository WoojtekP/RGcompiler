#pragma once
#include <memory>
#include <string_view>

#include <graph/Action.hpp>
namespace common
{
constexpr std::string_view PLAYER_WORD = "player";
constexpr std::string_view KEEPER_WORD = "keeper";
constexpr std::string_view KEEPER_CASTED_WORD = "static_cast<PlayerOrSystem>(keeper)";
constexpr std::string_view BEGIN_WORD = "begin";
constexpr std::string_view END_WORD = "end";
constexpr std::string_view PLAYER_TYPE_WORD = "Player";
constexpr std::string_view PLAYER_OR_SYSTEM_TYPE_WORD = "PlayerOrSystem";
constexpr std::string_view SCORE_TYPE_WORD = "Score";

inline bool isActionAssignmentToPlayer(const std::shared_ptr<IAction>& action)
{
    return action->getType() == ActionType::Assignment && action->getLeftSide() == PLAYER_WORD;
}

inline bool isActionAssignmentKeeperToPlayer(const std::shared_ptr<IAction>& action)
{
    return action->getType() == ActionType::Assignment && action->getLeftSide() == PLAYER_WORD &&
           (action->getRightSide() == KEEPER_CASTED_WORD || action->getRightSide() == KEEPER_WORD);
}

}  // namespace common
