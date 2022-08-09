#ifndef _RBG_RANDOM_GENERATOR_HPP
#define _RBG_RANDOM_GENERATOR_HPP

// TODO Make different structs
// TODO Add function for reals

#define RBG_RANDOM_GENERATOR 0
/**
 * RBG_RANDOM_GENERATOR:
 * 0 -- std::mt19937 with Lemire's method (unbiased)
 * 1 -- std::mt19937 with std::uniform_int_distribution (the standard unbiased method)
 * 2 -- Java's algorithm (lcg48, unbiased)
 * 3 -- lcg48 with multiplication method (biased)
 * 4 -- xorShift64 with multiplication method (biased)
 */
#include <cstdint>
//*********************************************************************
#if RBG_RANDOM_GENERATOR == 1

// The standard method
#include <random>
struct RBGRandomGenerator {
  std::mt19937 random_generator;
  RBGRandomGenerator(const unsigned long _seed): random_generator(_seed) {}
  unsigned int uniform_choice(const unsigned int upper_bound) {
    return std::uniform_int_distribution<unsigned int>(0,upper_bound-1)(random_generator);
  }
};
//*********************************************************************
#elif RBG_RANDOM_GENERATOR == 2

// An exact reimplementation of the standard Java generator
struct RBGRandomGenerator {
  uint64_t seed;

  RBGRandomGenerator(const uint64_t _seed): seed((_seed ^ 0x5DEECE66DUL) & ((1UL << 48) - 1)) {}

  uint32_t java_rand31() {
    seed = (seed * 0x5DEECE66DUL + 0xB) & ((1UL << 48) - 1);
    return seed >> (48 - 31);
  }

  unsigned int uniform_choice(uint32_t bound) {
    // assert(bound > 0 && bound <= (1U << 31));
    uint32_t boundm1 = bound-1;
    if ((bound & boundm1) == 0)
      return (bound * static_cast<uint64_t>(java_rand31())) >> 31;
    uint32_t bits, val;
    do {
        bits = java_rand31();
        val = bits % bound;
    } while ((bits-val+boundm1) & (1U << 31));
    return val;
  }
};
//*********************************************************************
#elif RBG_RANDOM_GENERATOR == 3

// Linear congruential 48 with the multiplication method (biased)
struct RBGRandomGenerator {
  uint64_t seed;
  RBGRandomGenerator(const unsigned long _seed): seed(_seed) {}

  uint64_t rand48() {
     seed = (0x5DEECE66DUL * seed + 0xBUL) & ((1UL << 48) - 1);
     return seed;
  }

  unsigned int uniform_choice(const unsigned int bound) {
     return ((rand48() >> 16) * uint64_t(bound)) >> 32;
  }
};
//*********************************************************************
#elif RBG_RANDOM_GENERATOR == 4

// XORShift64 with the multiplication method (biased)
struct RBGRandomGenerator {
  uint64_t seed;
  RBGRandomGenerator(const uint64_t _seed): seed(_seed) {}

  uint64_t xorShift64() {
    seed ^= (seed << 21);
    seed ^= (seed >> 35);
    seed ^= (seed << 4);
    return seed;
  }

  unsigned int uniform_choice(const unsigned int bound) {
     return ((xorShift64() >> 32) * uint64_t(bound)) >> 32;
  }
};
//*********************************************************************
#else

// Default for RBG.
// std::mt19937 with the Lemire's method.
// Source: https://www.pcg-random.org/posts/bounded-rands.html
#include <random>
struct RBGRandomGenerator {
  std::mt19937 random_generator;
  RBGRandomGenerator(const unsigned long seed): random_generator(seed) {}

  unsigned int uniform_choice(const uint32_t upper_bound) {
    uint32_t x = random_generator();
    uint64_t m = uint64_t(x) * uint64_t(upper_bound);
    uint32_t l = uint32_t(m);
    if (l < upper_bound) {
        uint32_t t = -upper_bound;
        if (t >= upper_bound) {
            t -= upper_bound;
            if (t >= upper_bound)
                t %= upper_bound;
        }
        while (l < t) {
            x = random_generator();
            m = uint64_t(x) * uint64_t(upper_bound);
            l = uint32_t(m);
        }
    }
    return m >> 32;
  }
};
//*********************************************************************
#endif
#endif

