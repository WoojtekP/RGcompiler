#ifndef _FAST_RANDOM_HPP
#define _FAST_RANDOM_HPP
#include <random>
#include <cstdint>
namespace fast_random {
  
/**
Unbiased:
fast_random::GenMTStd randomGenerator(1);
fast_random::GenJava randomGenerator(1);
fast_random::GenMTLemire randomGenerator(1);
fast_random::GenMTLemireTweak randomGenerator(1);
fast_random::GenMTLemireTweak2 randomGenerator(1);
Biased:
fast_random::GenLC48Mult randomGenerator(1);
fast_random::GenXORShift64Mult randomGenerator(1);
*/

//*********************************************************************

/**
 * The standard method (unbiased)
 */
template<class RG32> struct GenStd {
  RG32 rg;
  GenStd(const uint64_t _seed): rg(_seed) {}
  
  uint32_t rand_uint(const uint32_t bound) {
    return std::uniform_int_distribution<uint32_t>(0, bound-1)(rg);
  }
  
  double rand_double01() {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rg);
  }
  double rand_double(const double bound) {
    return std::uniform_real_distribution<double>(0.0, bound)(rg);
  }
};
using GenMTStd = GenStd<std::mt19937>;

/**
 * An exact reimplementation of the standard lc48 Java generator (unbiased)
 */
class GenJava {
  uint32_t java_rand31() {
    seed = (seed * 0x5DEECE66DUL + 0xB) & ((1UL << 48) - 1);
    return seed >> (48 - 31);
  }
public:
  uint64_t seed;

  GenJava(const uint64_t _seed): seed((_seed ^ 0x5DEECE66DUL) & ((1UL << 48) - 1)) {}

  uint32_t rand_uint(uint32_t bound) {
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
  
  double rand_double01() {
    return static_cast<double>(java_rand31()) / (1U << 31);
  }  
  double rand_double(const double bound) {
    return static_cast<double>(java_rand31()) / (1U << 31) * bound;
  }  
};

/**
 * Custom generator with Lemire's method
 * Source: https://www.pcg-random.org/posts/bounded-rands.html
 */
template<class RG32> struct GenLemire {
  RG32 rg;
  GenLemire(const uint64_t _seed): rg(_seed) {}

  uint32_t rand_uint(uint32_t bound) {
    uint32_t t = (-bound) % bound;
    uint64_t m;
    uint32_t l;
    do {
      uint32_t x = rg();
      m = uint64_t(x) * uint64_t(bound);
      l = uint32_t(m);
    } while (l < t);
    return m >> 32;
  }

  double rand_double01() {
    return static_cast<double>(rg()) / (static_cast<uint64_t>(1) << 32);
  }
  double rand_double(const double bound) {
    return static_cast<double>(rg()) / (static_cast<uint64_t>(1) << 32) * bound;
  }
};
using GenMTLemire = GenLemire<std::mt19937>;


/**
 * Custom generator with Lemire's method tweaked a little
 * Source: https://www.pcg-random.org/posts/bounded-rands.html
 */
template<class RG32> struct GenLemireTweak {
  RG32 rg;
  uint64_t seed;
  GenLemireTweak(const uint64_t _seed): rg(_seed) {seed = _seed;}

  uint32_t rand_uint(const uint32_t bound) {
    uint32_t x = rg();
    uint64_t m = uint64_t(x) * uint64_t(bound);
    uint32_t l = uint32_t(m);
    if (__builtin_expect(l < bound, false)) {
      uint32_t t = -bound % bound;
      while (l < t) {
        x = rg();
        m = uint64_t(x) * uint64_t(bound);
        l = uint32_t(m);
      }
    }
    return m >> 32;
  }
  
  double rand_double01() {
    return static_cast<double>(rg()) / (static_cast<uint64_t>(1) << 32);
  }
  double rand_double(const double bound) {
    return static_cast<double>(rg()) / (static_cast<uint64_t>(1) << 32) * bound;
  }
};
using GenMTLemireTweak = GenLemireTweak<std::mt19937>;

/**
 * Hard-coded LC48 with Lemire's method tweaked a little
 * Source: https://www.pcg-random.org/posts/bounded-rands.html
 */
struct GenLC48LemireTweak {
  uint64_t seed;
  GenLC48LemireTweak(const uint64_t _seed): seed(_seed) {}

  uint64_t rand48() {
     seed = (0x5DEECE66DUL * seed + 0xBUL) & ((1UL << 48) - 1);
     return seed;
  }
  
  uint32_t rand32() {return rand48() >> 16;}
  
  uint32_t rand_uint(const uint32_t bound) {
    uint32_t x = rand32();
    uint64_t m = uint64_t(x) * uint64_t(bound);
    uint32_t l = uint32_t(m);
    if (__builtin_expect(l < bound, false)) {
      uint32_t t = -bound % bound;
      while (l < t) {
        x = rand32();
        m = uint64_t(x) * uint64_t(bound);
        l = uint32_t(m);
      }
    }
    return m >> 32;
  }
  
  double rand_double01() {
    return static_cast<double>(rand32()) / (static_cast<uint64_t>(1) << 32);
  }
  double rand_double(const double bound) {
    return static_cast<double>(rand32()) / (static_cast<uint64_t>(1) << 32) * bound;
  }
};


using GenDefault = GenLC48LemireTweak;

//*********************************************************************

/**
 * LC48 with the multiplication method (biased)
 */
class GenLC48Mult {
  uint64_t rand48() {
     seed = (0x5DEECE66DUL * seed + 0xBUL) & ((1UL << 48) - 1);
     return seed;
  }
public:
  uint64_t seed;
  GenLC48Mult(const uint64_t _seed): seed(_seed) {}

  uint32_t rand_uint(const uint32_t bound) {
     return ((rand48() >> 16) * uint64_t(bound)) >> 32;
  }
  
  double rand_double01() {
    return static_cast<double>(rand48() >> 16) / (static_cast<uint64_t>(1) << 32);
  }  
  double rand_double(const double bound) {
    return static_cast<double>(rand48() >> 16) / (static_cast<uint64_t>(1) << 32) * bound;
  }  
};

/**
 * XORShift64 with the multiplication method (biased)
 */
class GenXORShift64Mult {
  uint64_t xorShift64() {
    seed ^= (seed << 21);
    seed ^= (seed >> 35);
    seed ^= (seed << 4);
    return seed;
  }
public:
  uint64_t seed;
  GenXORShift64Mult(const uint64_t _seed): seed(_seed) {}

  uint32_t rand_uint(const uint32_t bound) {
     return ((xorShift64() >> 32) * uint64_t(bound)) >> 32;
  }

  double rand_double01() {
    return static_cast<double>(xorShift64() >> 32) / (static_cast<uint64_t>(1) << 32);
  }  
  double rand_double(const double bound) {
    return static_cast<double>(xorShift64() >> 32) / (static_cast<uint64_t>(1) << 32) * bound;
  }  
};

//*********************************************************************
}
#endif
