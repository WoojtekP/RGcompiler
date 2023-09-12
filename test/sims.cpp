#include <iostream>
#include "fast_random.hpp"
#include <reasoner.hpp>
using uint = unsigned int;
using ulong = unsigned long;

fast_random::GenDefault randomGenerator(1);

reasoner::GameState initial;
reasoner::RgCache cache;
std::vector<reasoner::Move> moves;

ulong numSimulations;
ulong numStates = 0, minDepth = std::numeric_limits<ulong>::max(), maxDepth = 0;
ulong numMoves = 0, minMoves = std::numeric_limits<ulong>::max(), maxMoves = 0;
ulong sumScores[1+reasoner::PLAYERS_COUNT];

void exitWithError(const std::string msg) {std::cerr << msg << std::endl; exit(2);}

bool keeperCompletion(reasoner::GameState &state) {
  while (state.getCurrentPlayer() == reasoner::keeper) {
    if (state.isTerminal()) return false;
    state.getAllMoves(moves, cache);
    #ifndef NDEBUG
      if (moves.size() != 1) exitWithError("Keeper has " + std::to_string(moves.size()) + " moves in keeperCompletion");
    #endif
    state.applyMove(moves[0], cache);

    //state.applyAnyMove(cache);
  }
  return true;
}

void doSimulation() {
  reasoner::GameState state = initial;
  uint depth = 0;
  while (true) {
    #ifndef NDEBUG
      if (state.getCurrentPlayer() == reasoner::keeper) exitWithError("Keeper at the beginning of player loop");
    #endif
    
    state.getAllMoves(moves, cache);
    #ifndef NDEBUG
      if (moves.size() == 0) exitWithError("Player " + std::to_string(state.getCurrentPlayer()) + " has 0 moves");
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
    std::cerr << "Usage: " << argv[0] << " [number of simulations]" << std::endl;
    return 1;
  }

  [[maybe_unused]] bool initialNonterminal = keeperCompletion(initial);
  #ifndef NDEBUG
    if (!initialNonterminal) exitWithError("Initial state is terminal");
  #endif
  
  numSimulations = std::stoi(argv[1]);
  for (uint i = 0; i < numSimulations; i++) doSimulation();
  if (maxMoves == 0) maxMoves = minMoves;
  if (maxDepth == 0) maxDepth = minDepth;

  std::cout << std::fixed;
  std::cout.precision(2);
  std::cout << numStates << " " << minDepth << " " << maxDepth;
  std::cout << " " << numMoves << " " << minMoves << " " << maxMoves;
  for (uint player = 1; player <= reasoner::PLAYERS_COUNT; player++) std::cout << " " << sumScores[player];
  std::cout << std::endl;
  return 0;
}
