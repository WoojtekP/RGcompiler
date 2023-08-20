#include <iostream>
#include "fast_random.hpp"
#include <reasoner.hpp>
using uint = unsigned int;
using ulong = unsigned long;

constexpr uint MAX_DEPTH = 100;

reasoner::GameState initial;
reasoner::RgCache cache;
std::vector<reasoner::Move> moves[MAX_DEPTH];

uint maxDepth;
ulong numStates, numLeaves, numTerminals;

void exitError(const std::string msg) {std::cerr << msg << std::endl; exit(2);}

void keeperCompletion(reasoner::GameState &state, const uint depth) {
  while (state.getCurrentPlayer() == reasoner::keeper && !state.isTerminal()) {
    state.getAllMoves(moves[depth], cache);
    if (moves[depth].size() != 1) exitError("Keeper has " + std::to_string(moves[depth].size()) + " moves in keeperCompletion");
    state.applyMove(moves[depth][0], cache);
  }
}

void doPerft(reasoner::GameState &state, const uint depth) {
  numStates++;
  if (depth == 0 || state.isTerminal()) {
    if (depth == 0) numLeaves++;
    if (state.isTerminal()) numTerminals++;
    return;
  }
  state.getAllMoves(moves[depth], cache);
  if (moves[depth].size() == 0) exitError("Player " + std::to_string(state.getCurrentPlayer()) + " has 0 moves");
  for (uint i = 0; i < moves[depth].size(); i++) {
    reasoner::GameState nextState = state;
    nextState.applyMove(moves[depth][i], cache);
    keeperCompletion(nextState, 0);
    doPerft(nextState, depth-1);
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " depth" << std::endl;
    return 1;
  }

  keeperCompletion(initial, 0);

  maxDepth = std::stoi(argv[1]);

  doPerft(initial, maxDepth);

  std::cout << std::fixed;
  std::cout << numLeaves << " " << numStates << " " << numTerminals << std::endl;
  return 0;
}
