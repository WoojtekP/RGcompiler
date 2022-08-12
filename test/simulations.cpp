#include <iostream>
#include <chrono>
#include "fast_random.hpp"
#include "../reasoner.hpp"
using uint = unsigned int;
using ulong = unsigned long;

RBGRandomGenerator randomGenerator(1);

GameState initial;
std::vector<Move> moves;

ulong numSimulations;
ulong numStates = 0, minDepth = std::numeric_limits<ulong>::max(), maxDepth = 0;
ulong numMoves = 0, minMoves = std::numeric_limits<ulong>::max(), maxMoves = 0;

void exitError(const std::string msg) {std::cerr << msg << std::endl; exit(2);}

void keeperCompletion(GameState &state) {
  while (state.getCurrentPlayer() == keeper && !state.isTerminal()) {
    state.getAllMoves(moves);
    if (moves.size() != 1) exitError("Keeper has " + std::to_string(moves.size()) + " moves");
    state.applyMove(moves[0]);
  }
}

void doSimulation() {
  GameState state = initial;
  uint depth = 0;
  while (!state.isTerminal()) {
    state.getAllMoves(moves);
    if (state.getCurrentPlayer() == keeper) {
      if (moves.size() != 1) exitError("Keeper has " + std::to_string(moves.size()) + " moves");
    } else {
      if (moves.size() == 0) exitError("Player has 0 moves");
      depth++;
      numMoves += moves.size();
      if (moves.size() < minMoves) minMoves = moves.size();
      if (moves.size() > maxMoves) maxMoves = moves.size();
    }
    state.applyMove(moves[randomGenerator.uniform_choice(moves.size())]);
  }
  numStates += depth;
  if (depth < minDepth) minDepth = depth;
  if (depth > maxDepth) maxDepth = depth;
}

int main(int argc, char** argv) {
  if (argc != 2) {
      std::cerr << "usage: " << argv[0] << " [number of simulations]" << std::endl;
      return 1;
  }

  keeperCompletion(initial);

  numSimulations = std::stoi(argv[1]);

  std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  for (uint i = 0; i < numSimulations; i++)
    doSimulation();
  std::chrono::steady_clock::time_point endTime(std::chrono::steady_clock::now());
  long double seconds = std::chrono::duration<long double>(endTime-startTime).count();

  std::cout << std::fixed;
  std::cout << "time: " << seconds << " sec" << std::endl;
  std::cout << "simulations: " << numSimulations << " (" << numSimulations / seconds << " simulations/sec)" << std::endl;
  std::cout << "states: " << numStates << " (" << numStates / seconds << " states/sec)" << std::endl;
  std::cout << "depth: min " << minDepth << " avg " << static_cast<long double>(numStates) / numSimulations << " max " << maxDepth << std::endl;
  std::cout << "moves: min " << minMoves << " avg " << static_cast<long double>(numMoves) / numStates << " max " << maxMoves << std::endl;
  return 0;
}
