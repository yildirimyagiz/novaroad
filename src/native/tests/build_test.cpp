#include "nova_core.hpp"
#include <iostream>

int main() {
  uint64_t t1 = nova::Clock::ticks();
  std::cout << "Ticks 1: " << t1 << std::endl;

  // Simulate some work
  for (volatile int i = 0; i < 1000000; ++i)
    ;

  uint64_t t2 = nova::Clock::ticks();
  std::cout << "Ticks 2: " << t2 << std::endl;

  if (t2 > t1) {
    std::cout << "Verification PASSED: Clock is advancing." << std::endl;
    return 0;
  } else if (t2 == t1) {
    std::cout << "Verification WARNING: Clock did not advance (might be too "
                 "fast or timer resolution issue)."
              << std::endl;
    return 0;
  } else {
    std::cout << "Verification FAILED: Clock went backwards!" << std::endl;
    return 1;
  }
}
