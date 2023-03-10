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
    het::hkeyvalue hkv(1);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_single_ctor);

static void tuple_keyvalue_single_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    std::tuple<int> v = std::make_tuple(1);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(v);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_keyvalue_single_ctor);

static void generic_keyvalue_single_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    int i = 1;

    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
  }
}
// Register the function as a benchmark
BENCHMARK(generic_keyvalue_single_ctor);

static void het_keyvalue_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    het::hkeyvalue hkv(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_ctor);

static void tuple_keyvalue_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    std::tuple<int, float, double, char, std::string_view, std::string> values =
        std::make_tuple(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(values);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_keyvalue_ctor);

static void generic_keyvalue_ctor(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    int i = 1;
    float f = 1.2f;
    double d = 3.;
    char c = 'c';
    std::string_view sv = "stringview"sv;
    std::string s = "string"s;

    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
    benchmark::DoNotOptimize(f);
    benchmark::DoNotOptimize(d);
    benchmark::DoNotOptimize(c);
    benchmark::DoNotOptimize(sv);
    benchmark::DoNotOptimize(s);
  }
}
// Register the function as a benchmark
BENCHMARK(generic_keyvalue_ctor);

static void het_keyvalue_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  het::hkeyvalue hkv(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
  for (auto _ : state) {
    auto i = het::get<int>(hkv);
    auto f = het::get<float>(hkv);
    auto d = het::get<double>(hkv);
    auto c = het::get<char>(hkv);
    auto sv = het::get<std::string_view>(hkv);
    auto s = het::get<std::string>(hkv);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hkv);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_access);

static void tuple_keyvalue_access(benchmark::State& state) {
  std::tuple<int, float, double, char, std::string_view, std::string> values =
      std::make_tuple(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    auto i = std::get<int>(values);
    auto f = std::get<float>(values);
    auto d = std::get<double>(values);
    auto c = std::get<char>(values);
    auto sv = std::get<std::string_view>(values);
    auto s = std::get<std::string>(values);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
    benchmark::DoNotOptimize(f);
    benchmark::DoNotOptimize(d);
    benchmark::DoNotOptimize(c);
    benchmark::DoNotOptimize(sv);
    benchmark::DoNotOptimize(s);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_keyvalue_access);

static void generic_keyvalue_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  int i = 1;
  float f = 1.2f;
  double d = 3.;
  char c = 'c';
  std::string_view sv = "stringview"sv;
  std::string s = "string"s;
  for (auto _ : state) {
    int i1 = i;
    float f1 = f;
    double d1 = d;
    char c1 = c;
    std::string_view sv1 = sv;
    std::string s1 = s;
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i1);
    benchmark::DoNotOptimize(f1);
    benchmark::DoNotOptimize(d1);
    benchmark::DoNotOptimize(c1);
    benchmark::DoNotOptimize(sv1);
    benchmark::DoNotOptimize(s1);
  }
}
// Register the function as a benchmark
BENCHMARK(generic_keyvalue_access);

static void het_keyvalue_single_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  het::hkeyvalue hkv(1);
  for (auto _ : state) {
    auto i = het::get<int>(hkv);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
  }
}
// Register the function as a benchmark
BENCHMARK(het_keyvalue_single_access);

static void tuple_keyvalue_single_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  std::tuple<int> v = std::make_tuple(1);
  for (auto _ : state) {
    auto i = std::get<int>(v);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_keyvalue_single_access);

static void generic_keyvalue_single_access(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  int i = 1;
  for (auto _ : state) {
    int i1 = i;
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(i1);
  }
}
// Register the function as a benchmark
BENCHMARK(generic_keyvalue_single_access);

BENCHMARK_MAIN();


