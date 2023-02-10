//
// Created by yuri on 2/2/23.
//

#include <benchmark/benchmark.h>

#include "het/het.h"

using namespace std::string_view_literals;
using namespace std::string_literals;

static void het_container_push_back(benchmark::State& state) {
  het::hvector hc;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    hc.template emplace_back<int>(1);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hc);
  }
}
// Register the function as a benchmark
BENCHMARK(het_container_push_back);

static void tuple_container_push_back(benchmark::State& state) {
  std::vector<std::tuple<int>> values;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    values.emplace_back(1);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(values);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_container_push_back);

static void generic_container_push_back(benchmark::State& state) {
  std::vector<int> values;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    values.push_back(1);

    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(values);
  }
}
// Register the function as a benchmark
BENCHMARK(generic_container_push_back);

static void het_container_push_back_tuple(benchmark::State& state) {
  het::hvector hc;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    hc.push_back(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hc);
  }
}
// Register the function as a benchmark
BENCHMARK(het_container_push_back_tuple);

static void tuple_container_push_back_tuple(benchmark::State& state) {
  std::vector<std::tuple<int, float, double, char, std::string_view, std::string>> values;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    values.emplace_back(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(values);
  }
}
// Register the function as a benchmark
BENCHMARK(tuple_container_push_back_tuple);

static void het_container_access(benchmark::State& state) {
  het::hvector hc;
  // Code inside this loop is measured repeatedly
  std::size_t k = 0;
  for (auto _ : state) {
    state.PauseTiming();
    hc.push_back(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    state.ResumeTiming();
    auto i = hc.fraction<int>().at(k);
    auto f = hc.fraction<float>().at(k);
    auto d = hc.fraction<double>().at(k);
    auto c = hc.fraction<char>().at(k);
    auto sv = hc.fraction<std::string_view>().at(k);
    auto s = hc.fraction<std::string>().at(k);
    k++;
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
BENCHMARK(het_container_access);

static void het_container_visit_access(benchmark::State& state) {
  het::hvector hc;
  auto visitor = hc.visit<int, float, double, char, std::string_view, std::string>();
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    state.PauseTiming();
    hc.push_back(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    state.ResumeTiming();
    visitor([](auto) {return het::VisitorReturn::Continue;});
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(hc);
  }
}
// Register the function as a benchmark
BENCHMARK(het_container_visit_access);

static void tuple_container_access(benchmark::State& state) {
  std::vector<std::tuple<int, float, double, char, std::string_view, std::string>> values;
  std::size_t k = 0;
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    state.PauseTiming();
    values.emplace_back(1, 1.2f, 3., 'c', "stringview"sv, "string"s);
    state.ResumeTiming();
    auto t = values.at(k);
    auto i = std::get<int>(t);
    auto f = std::get<float>(t);
    auto d = std::get<double>(t);
    auto c = std::get<char>(t);
    auto sv = std::get<std::string_view>(t);
    auto s = std::get<std::string>(t);
    k++;
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
BENCHMARK(tuple_container_access);
