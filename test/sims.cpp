#include <iostream>
#include <chrono>
#include "fast_random.hpp"
#include <reasoner.hpp>
using uint = unsigned int;
using ulong = unsigned long;

#ifndef USE_TIME
#define USE_TIME 0
#endif

#ifdef NDEBUG
constexpr bool BENCHMARK = true;
#else
constexpr bool BENCHMARK = false;
#endif

constexpr bool KEEPER_APPLY_ANY_MOVE = true;

fast_random::GenDefault randomGenerator(1);

reasoner::GameState initial;
reasoner::RgCache cache;
std::vector<reasoner::Move> moves;

ulong numSims;
ulong numStates = 0, minDepth = std::numeric_limits<ulong>::max(), maxDepth = 0;
ulong numMoves = 0, minMoves = std::numeric_limits<ulong>::max(), maxMoves = 0;
ulong sumScores[2+reasoner::PLAYERS_COUNT], minScores[2+reasoner::PLAYERS_COUNT], maxScores[2+reasoner::PLAYERS_COUNT];

void exitWithError(const reasoner::GameState &state, const std::string msg) {
  std::cerr << msg << std::endl;
  std::cerr << state.getStateDescription();
  exit(2);
}

bool keeperCompletion(reasoner::GameState &state) {
  while (true) {
    switch (state.getCurrentPlayer()) {
      case reasoner::keeper: {
        if (state.isTerminal()) return false;
        if constexpr(KEEPER_APPLY_ANY_MOVE) {
          state.applyAnyMove(cache);
        } else {
          state.getAllMoves(moves, cache);
          if constexpr(!BENCHMARK) {
            if (moves.size() != 1) exitWithError(state, "Keeper has " + std::to_string(moves.size()) + " moves in keeperCompletion");
          }
          state.applyMove(moves[0], cache);
        }
        break;
      }
      case reasoner::random: {
        state.getAllMoves(moves, cache);
        if constexpr(!BENCHMARK) {
          if (moves.size() == 0) exitWithError(state, "Random has no move in keeperCompletion");
        }
        state.applyMove(moves[randomGenerator.rand_uint(moves.size())], cache);
        break;
      }
      default: return true;
    }
  }
}

void doSimulation() {
  reasoner::GameState state = initial;
  uint depth = 0;

  while (true) {
    if constexpr(!BENCHMARK) {
      if (state.getCurrentPlayer() == reasoner::keeper) exitWithError(state, "Keeper at the beginning of player loop");
    }

    state.getAllMoves(moves, cache);

    if constexpr(!BENCHMARK) {
      if (moves.size() == 0) exitWithError(state, "Player " + std::to_string(state.getCurrentPlayer()) + " has 0 moves");
    }

    depth++;
    if constexpr(!BENCHMARK) {
      numMoves += moves.size();
      if (moves.size() < minMoves) minMoves = moves.size(); else
      if (moves.size() > maxMoves) maxMoves = moves.size();
    }
    state.applyMove(moves[randomGenerator.rand_uint(moves.size())], cache);
    if (!keeperCompletion(state)) break;
  }

  numStates += depth;
  if constexpr(!BENCHMARK) {
    if (depth < minDepth) minDepth = depth; else
    if (depth > maxDepth) maxDepth = depth;
    for (uint player = 2; player <= 1 + reasoner::PLAYERS_COUNT; player++) {
      uint score = state.getPlayerScore(player);
      sumScores[player] += score;
      if (score < minScores[player]) minScores[player] = score; else
      if (score > maxScores[player]) maxScores[player] = score;
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    if constexpr(USE_TIME) {
      std::cerr << "Usage: " << argv[0] << " [time in ms]\n";
    } else {
      std::cerr << "Usage: " << argv[0] << " [number of simulations]\n";
    }
    std::cerr << "Compiled in " << (BENCHMARK ? "benchmark" : "test") << " mode" << std::endl;
    return 1;
  }

  [[maybe_unused]] bool initialNonterminal = keeperCompletion(initial);
  if constexpr(!BENCHMARK) {
    if (!initialNonterminal) exitWithError(initial, "Initial state is terminal");
  }

  std::chrono::steady_clock::time_point endTime, startTime;
  if constexpr(USE_TIME) {
    startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point plannedEndTime = startTime + std::chrono::milliseconds(std::stoi(argv[1]));
    for (numSims = 1; ; numSims++) {
      doSimulation();
      endTime = std::chrono::steady_clock::now();
      if (endTime >= plannedEndTime) break;
    }
  } else {
    numSims = std::stoi(argv[1]);
    startTime = std::chrono::steady_clock::now();
    for (uint i = 0; i < numSims; i++) doSimulation();
    endTime = std::chrono::steady_clock::now();
  }

  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count() << " " << numSims << " " << numStates;
  if constexpr (!BENCHMARK) {
    std::cout << std::fixed; std::cout.precision(2);
    if (maxMoves == 0) maxMoves = minMoves;
    if (maxDepth == 0) maxDepth = minDepth;
    std::cout << " " << minDepth << " " << maxDepth << " " << numMoves << " " << minMoves << " " << maxMoves;
    for (uint player = 2; player <= 1 + reasoner::PLAYERS_COUNT; player++) {
      std::cout << " " << sumScores[player] << " " << minScores[player] << " " << maxScores[player];
    }
  }
  std::cout << std::endl;
  return 0;
}
