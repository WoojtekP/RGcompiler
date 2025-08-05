#include <iostream>
#include <chrono>
#include <typeinfo>
#include <cassert>
#include "fast_random.hpp"
using uint = unsigned int;
using ulong = unsigned long;

template<class Gen> void doBenchmark() {
  Gen randomGenerator(1);
  std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  uint sum = 0;
  double sumd = 0.0;
  constexpr uint COUNT = 1000*1000*1000;
  for (uint i = 0; i < 1000*1000*1000; i++) {
    sum += randomGenerator.rand_uint(1 + i%1000);
    double d = randomGenerator.rand_double01();
    assert(d >= 0.0 && d < 1.0);
    sumd += d;
  }
  std::chrono::steady_clock::time_point endTime(std::chrono::steady_clock::now());
  long double seconds = std::chrono::duration<long double>(endTime-startTime).count();
  
  std::cout << std::fixed;
  std::cout << typeid(randomGenerator).name() << std::endl;
  std::cout << "time: " << seconds << " sec" << std::endl;
  std::cout << "sum: " << sum << std::endl;
  std::cout << "sumd: " << sumd / COUNT << std::endl;
}

int main() {
  doBenchmark<fast_random::GenMTStd>();
  doBenchmark<fast_random::GenJava>();
  doBenchmark<fast_random::GenMTLemire>();
  doBenchmark<fast_random::GenMTLemireTweak>();
  doBenchmark<fast_random::GenLC48LemireTweak>();

  doBenchmark<fast_random::GenLC48Mult>();
  doBenchmark<fast_random::GenXORShift64Mult>();
  return 0;
}
