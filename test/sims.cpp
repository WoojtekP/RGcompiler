#include <iostream>
#include <chrono>
#include "fast_random.hpp"
#include <reasoner.hpp>
using uint = unsigned int;
using ulong = unsigned long;

#ifndef USE_TIME
#define USE_TIME 0
#endif

#define KEEPER_APPLY_ANY_MOVE 1

fast_random::GenDefault randomGenerator(1);

reasoner::GameState initial;
reasoner::RgCache cache;
std::vector<reasoner::Move> moves;

ulong numSims;
ulong numStates = 0, minDepth = std::numeric_limits<ulong>::max(), maxDepth = 0;
ulong numMoves = 0, minMoves = std::numeric_limits<ulong>::max(), maxMoves = 0;
ulong sumScores[1+reasoner::PLAYERS_COUNT];

void exitWithError(const reasoner::GameState &state, const std::string msg)
{
  std::cerr << msg << std::endl;
  std::cerr << state.getStateDescription();
  exit(2);
}
reasoner::Move EMPTY_MOVE;

bool keeperCompletion(reasoner::GameState &state) {
  while (state.getCurrentPlayer() <= 0) {
    if (state.getCurrentPlayer() == reasoner::keeper) {
      //std::cerr << "keeper" << std::endl;
      if (state.isTerminal()) return false;
      if constexpr(KEEPER_APPLY_ANY_MOVE) {
        state.applyAnyMove(cache);
      } else {
        state.getAllMoves(moves, cache);
        #ifndef NDEBUG
          if (moves.size() != 1) exitWithError(state, "Keeper has " + std::to_string(moves.size()) + " moves in keeperCompletion");
        #endif
        state.applyMove(moves[0], cache);
      }
    } else {// random
      //std::cerr << "random" << std::endl;
      state.getAllMoves(moves, cache);
      #ifndef NDEBUG
        if (moves.size() == 0) exitWithError(state, "Random has no move in keeperCompletion");
      #endif
      state.applyMove(moves[randomGenerator.rand_uint(moves.size())], cache);
    }
  }
  return true;
}

void doSimulation() {
  reasoner::GameState state = initial;
  uint depth = 0;
  while (true) {
    //std::cerr << "depth " << depth << " player " << state.getCurrentPlayer() << std::endl;
    #ifndef NDEBUG
      if (state.getCurrentPlayer() == reasoner::keeper) exitWithError(state, "Keeper at the beginning of player loop");
    #endif
    
    state.getAllMoves(moves, cache);
    #ifndef NDEBUG
      if (moves.size() == 0) exitWithError(state, "Player " + std::to_string(state.getCurrentPlayer()) + " has 0 moves");
    #endif
    depth++;
    numMoves += moves.size();
    if (moves.size() < minMoves) minMoves = moves.size(); else
    if (moves.size() > maxMoves) maxMoves = moves.size();
    state.applyMove(moves[randomGenerator.rand_uint(moves.size())], cache);
    
    if (!keeperCompletion(state)) break;
  }
  numStates += depth;
  if (depth < minDepth) minDepth = depth; else
  if (depth > maxDepth) maxDepth = depth;
  for (uint player = 1; player <= reasoner::PLAYERS_COUNT; player++) sumScores[player] += state.getPlayerScore(player);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    if constexpr(USE_TIME) {
      std::cerr << "Usage: " << argv[0] << " [time in ms]" << std::endl;
    } else {
      std::cerr << "Usage: " << argv[0] << " [number of simulations]" << std::endl;
    }
    return 1;
  }

  [[maybe_unused]] bool initialNonterminal = keeperCompletion(initial);
  #ifndef NDEBUG
    if (!initialNonterminal) exitWithError(initial, "Initial state is terminal");
  #endif
  
  if constexpr(USE_TIME) {
    std::chrono::duration simulation_duration = std::chrono::milliseconds(std::stoi(argv[1]));
    std::chrono::steady_clock::time_point end_time;
    std::chrono::steady_clock::time_point start_time(std::chrono::steady_clock::now());
    std::chrono::steady_clock::time_point planned_end_time = start_time + simulation_duration;
    for (numSims = 1; ; numSims++) {
      doSimulation();
      end_time = std::chrono::steady_clock::now();
      if (end_time >= planned_end_time) break;
    }
  } else {
    numSims = std::stoi(argv[1]);
    for (uint i = 0; i < numSims; i++) doSimulation();
  }
  
  if (maxMoves == 0) maxMoves = minMoves;
  if (maxDepth == 0) maxDepth = minDepth;

  std::cout << std::fixed; std::cout.precision(2);
  std::cout << numSims << " " << numStates << " " << minDepth << " " << maxDepth;
  std::cout << " " << numMoves << " " << minMoves << " " << maxMoves;
  for (uint player = 1; player <= reasoner::PLAYERS_COUNT; player++) std::cout << " " << sumScores[player];
  std::cout << std::endl;
  return 0;
}
