#include <iostream>
#include <chrono>
#include "fast_random.hpp"
#include "../reasoner.hpp"
using uint = unsigned int;
using ulong = unsigned long;

RBGRandomGenerator randomGenerator(1);

GameState initial;
std::vector<Move> moves[100];

uint maxDepth;
ulong numStates, numLeaves, numTerminals;

void exitError(const std::string msg) {std::cerr << msg << std::endl; exit(2);}

void keeperCompletion(GameState &state, const uint depth) {
  while (state.getCurrentPlayer() == keeper) {
    state.getAllMoves(moves[depth]);
    if (moves[depth].size() != 1) exitError("Keeper has " + std::to_string(moves[depth].size()) + " moves");
    state.applyMove(moves[depth][0]);
  }
}

void doPerft(GameState &state, const uint depth) {
  numStates++;
  if (depth == 0) {numLeaves++; return;}
  state.getAllMoves(moves[depth]);
  if (moves[depth].size() == 0) exitError("Player has 0 moves");
  for (uint i = 0; i < moves[depth].size(); i++) {
    GameState nextState = state;
    nextState.applyMove(moves[depth][i]);
    keeperCompletion(nextState, 0);
    doPerft(nextState, depth-1);
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
      std::cerr << "usage: " << argv[0] << " [number of simulations]" << std::endl;
      return 1;
  }

  keeperCompletion(initial, 0);

  maxDepth = std::stoi(argv[1]);

  std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  doPerft(initial, maxDepth);
  std::chrono::steady_clock::time_point endTime(std::chrono::steady_clock::now());
  long double seconds = std::chrono::duration<long double>(endTime-startTime).count();

  std::cout << std::fixed;
  std::cout << "time: " << seconds << " sec" << std::endl;
  std::cout << "states: " << numStates << " (" << numStates / seconds << " states/sec)" << std::endl;
  std::cout << "terminals: " << numTerminals << std::endl;
  std::cout << "leaves: " << numLeaves << std::endl;
  return 0;
}
