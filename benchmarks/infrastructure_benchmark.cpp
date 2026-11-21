#include <benchmark/benchmark.h>

#include "suborno/core/project.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

static void BenchmarkInfrastructureAbiVersion(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(suborno::project_abi_version());
    }
}

BENCHMARK(BenchmarkInfrastructureAbiVersion);

static void BenchmarkPhoneticTransliteration(benchmark::State& state) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    for (auto _ : state) {
        auto output = layout.transliterate("sôngkhya");
        benchmark::DoNotOptimize(output);
    }
}

static void BenchmarkPhoneticCandidateGeneration(benchmark::State& state) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    for (auto _ : state) {
        auto candidates = layout.generate_candidates("ch sh");
        benchmark::DoNotOptimize(candidates);
    }
}

BENCHMARK(BenchmarkPhoneticTransliteration);
BENCHMARK(BenchmarkPhoneticCandidateGeneration);
BENCHMARK_MAIN();
