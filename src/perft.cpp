#include <chrono>
#include <iostream>

#include "reasoner.hpp"

typedef unsigned int uint;
typedef unsigned long ulong;
constexpr uint MAX_DEPTH = 100;

game_state initial_state;
ulong states_count, leaves_count;
std::vector<Move> legal_moves[MAX_DEPTH];

void perft_state_at_depth(game_state& state, uint depth)
{
    if (depth == 0 and state.get_current_player() != keeper)
    {
        ++states_count;
        ++leaves_count;
        return;
    }
    else
    {
        if (state.get_current_player() == keeper)
        {
            auto any_move = state.apply_any_move(cache);
            if (any_move)
                return perft_state_at_depth(state, depth);
            else
            {
                ++states_count;
                if (depth == 0)
                    ++leaves_count;
                return;
            }
        }
        else
        {
            ++states_count;
            state.get_all_moves(legal_moves[depth]);
            for (const auto& el : legal_moves[depth])
            {
                auto temp_state = state;
                temp_state.apply_move(el);
                perft_state_at_depth(temp_state, depth - 1);
            }
        }
    }
}

void perft(uint depth)
{
    perft_state_at_depth(initial_state, depth);
}

double count_per_sec(ulong count, ulong ms)
{
    return static_cast<double>(count) / static_cast<double>(ms) * 1000.0;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " [depth]" << std::endl;
        return 1;
    }
    while (initial_state.get_current_player() == keeper)
    {
        auto any_move = initial_state.apply_any_move(cache);
        if (not any_move)
            return 0;
    }

    uint depth = std::stoi(argv[1]);
    std::chrono::steady_clock::time_point start_time(std::chrono::steady_clock::now());
    perft(depth);
    std::chrono::steady_clock::time_point end_time(std::chrono::steady_clock::now());

    ulong ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "time: " << ms << " ms" << std::endl;
    std::cout << "perft: " << leaves_count << std::endl;
    std::cout << "number of states: " << states_count << " (" << std::fixed << count_per_sec(states_count, ms)
              << " states/sec)" << std::endl;
    return 0;
}
