#include "benchmark/benchmark.h"

static void dummy_bench(benchmark::State &state) {
  int value = 0;
  for (auto _ : state) {
    value += 1;
    benchmark::DoNotOptimize(value);
  }
}

BENCHMARK(dummy_bench);
