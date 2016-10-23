#include <fusca.h>
#include <x86intrin.h>
#include <cstdio>
#include <chrono>

int main(int argc, char *argv[])
{
  auto tscclock = []{ return __rdtsc(); };
  auto slowclock = [] { return std::chrono::high_resolution_clock::now(); };
  auto adapter = make_fusca(slowclock, tscclock);

  for(int i = 0; i < 100; i++)
  {
    auto delta = adapter() - slowclock();
    printf("Delta: %llu us\n", std::chrono::duration_cast<std::chrono::milliseconds>(delta).count());
  }

  return 0;
}
