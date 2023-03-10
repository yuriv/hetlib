//
// Created by yuri on 2/2/23.
//

#include <benchmark/benchmark.h>

#include <tuple>

#include "het/het_keyvalue.h"

using namespace std::string_view_literals;
using namespace std::string_literals;

static void het_keyvalue_single_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    het::hkeyvalue hkv(std::make_pair(1, 1));
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_single_ctor);

static void het_keyvalue_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    het::hkeyvalue hkv(std::make_pair(1, 1), std::make_pair(1.2f, 2.f), std::make_pair(3., 3.),
                       std::make_pair(2, 'c'), std::make_pair(3, "stringview"sv), std::make_pair(4, "string"s));
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_ctor);

static void het_keyvalue_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  het::hkeyvalue hkv(std::make_pair(1, 1), std::make_pair(1.2f, 2.f), std::make_pair(3., 3.),
                     std::make_pair(2, 'c'), std::make_pair(3, "stringview"sv), std::make_pair(4, "string"s));
  for (auto _ : state) {
    auto i = het::to_tuple<int>(hkv, 1);
    auto f = het::to_tuple<float>(hkv, 1.2f);
    auto d = het::to_tuple<double>(hkv, 3.);
    auto c = het::to_tuple<char>(hkv, 2);
    auto sv = het::to_tuple<std::string_view>(hkv, 3);
    auto s = het::to_tuple<std::string>(hkv, 4);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_access);


static void het_keyvalue_single_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  het::hkeyvalue hkv(std::make_pair(1, 1));
  for (auto _ : state) {
    auto i = het::to_tuple<int>(hkv, 1);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_single_access);
