#include <fusca.h>
#include <x86intrin.h>
#include <cstdio>
#include <chrono>

int main(int argc, char *argv[])
{
  auto fastclock = [] { return std::chrono::high_resolution_clock::now(); };
  auto slowclock = [] { return std::chrono::high_resolution_clock::now(); };
  auto adapter = fusca::make(slowclock, fastclock);

  for(int i = 0; i < 100; i++)
  {
    auto st1 = slowclock();
    auto st2 = slowclock();
    auto ft1 = fastclock();
    auto ft2 = fastclock();
    auto at1 = adapter();
    auto at2 = slowclock();
    auto fdel = ft2 - ft1;
    auto sdel = st2 - st1;
    auto adel = at2 - at1;
    printf("Fast Delta: %.4f ticks Slow Delta %.4f us Adapter to Slow Delta: %.4f us\n",
      std::chrono::duration_cast<std::chrono::nanoseconds>(fdel).count()/1000.0,
      std::chrono::duration_cast<std::chrono::nanoseconds>(sdel).count()/1000.0,
      std::chrono::duration_cast<std::chrono::nanoseconds>(adel).count()/1000.0);
  }

  return 0;
}
