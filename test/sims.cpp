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

void exitError(const std::string msg) {std::cerr << msg << std::endl; exit(2);}

void keeperCompletion(reasoner::GameState &state) {
  while (state.getCurrentPlayer() == reasoner::keeper && !state.isTerminal()) {
    state.getAllMoves(moves, cache);
    if (moves.size() != 1) exitError("Keeper has " + std::to_string(moves.size()) + " moves in keeperCompletion");
    state.applyMove(moves[0]);
  }
}

void doSimulation() {
  reasoner::GameState state = initial;
  uint depth = 0;
  while (!state.isTerminal()) {
    state.getAllMoves(moves, cache);
    if (state.getCurrentPlayer() == reasoner::keeper) {
      if (moves.size() != 1) exitError("Keeper has " + std::to_string(moves.size()) + " moves");
    } else {
      if (moves.size() == 0) exitError("Player " + std::to_string(state.getCurrentPlayer()) + " has 0 moves");
      depth++;
      numMoves += moves.size();
      if (moves.size() < minMoves) minMoves = moves.size();
      if (moves.size() > maxMoves) maxMoves = moves.size();
    }
    state.applyMove(moves[randomGenerator.rand_uint(moves.size())]);
  }
  numStates += depth;
  if (depth < minDepth) minDepth = depth;
  if (depth > maxDepth) maxDepth = depth;
  for (uint player = 1; player <= reasoner::PLAYERS_COUNT; player++) sumScores[player] += state.getPlayerScore(player);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " [number of simulations]" << std::endl;
    return 1;
  }

  keeperCompletion(initial);
  numSimulations = std::stoi(argv[1]);
  for (uint i = 0; i < numSimulations; i++) doSimulation();

  std::cout << std::fixed;
  std::cout.precision(2);
  std::cout << numStates << " " << minDepth << " " << maxDepth;
  std::cout << " " << numMoves << " " << minMoves << " " << maxMoves;
  for (uint player = 1; player <= reasoner::PLAYERS_COUNT; player++) std::cout << " " << sumScores[player];
  std::cout << std::endl;
  //std::cout << "simulations: " << numSimulations << " (" << numSimulations / seconds << " simulations/sec)" << std::endl;
  //std::cout << "states: " << numStates << " (" << numStates / seconds << " states/sec)" << std::endl;
  //std::cout << "depth: min " << minDepth << " avg " << static_cast<long double>(numStates) / numSimulations << " max " << maxDepth << std::endl;
  //std::cout << "moves: min " << minMoves << " avg " << static_cast<long double>(numMoves) / numStates << " max " << maxMoves << std::endl;
  //std::cout << "scores: avg";
  //for (uint player = 1; player <= 2; player++) std::cout << " " << static_cast<long double>(sumScores[player]) / numSimulations;
  return 0;
}
