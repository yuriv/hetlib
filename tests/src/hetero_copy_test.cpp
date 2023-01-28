//
// Created by yuri on 9/27/21.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <iostream>

#include "het/het.h"

TEST_CASE("heterogeneous container copy test") {
  het::hvector h;
  h.push_back<int>(123123);
  h.push_back<float>(1.11111f);

  CHECK_EQ(h.at<int>(0), 123123);
  CHECK_EQ(h.at<float>(0), 1.11111f);

  het::hvector w;
  w.push_back<int>(123);
  w.push_back<int>(1234);
  w.push_back<int>(12345);
  w.push_back<int>(123456);
  w.push_back<int>(1234567);
  w.push_back(3.14);
  w.push_back(3.141);
  w.push_back(3.1412);
  w.push_back(3.14123);
  w.push_back(3.141234);
  CHECK_EQ(w.fraction<int>(), std::vector<int>{123, 1234, 12345, 123456, 1234567});
  CHECK_EQ(w.fraction<double>(), std::vector<double>{3.14, 3.141, 3.1412, 3.14123, 3.141234});

  het::hvector ww;
  ww.push_back<int>(42);
  ww.push_back<int>(421);
  ww.push_back<int>(4212);
  ww.push_back<int>(42123);
  ww.push_back<int>(421234);
  ww.push_back(6.28);
  ww.push_back(6.281);
  ww.push_back(6.2812);
  ww.push_back(6.28123);
  ww.push_back(6.281234);
  ww.push_back(6.2812345);
  ww.push_back(6.28123456);

  w.push_back(ww);
  CHECK_EQ(w.fraction<int>(), std::vector<int>{42, 421, 4212, 42123, 421234});
  CHECK_EQ(w.fraction<double>(), std::vector<double>{6.28, 6.281, 6.2812, 6.28123, 6.281234, 6.2812345, 6.2812346});
  CHECK_EQ(w.at<het::hvector>(0), std::vector<double>{6.28, 6.281, 6.2812, 6.28123, 6.281234, 6.2812345, 6.2812346});

  h.push_back(w);
  h.push_back(w);
  h.push_back(w);
  h.push_back(std::move(w));

  het::hvector w0;
  w0 = h;

  std::cout << h.size() << "\n";
  //std::cout << h.at<int>(0) << "\n";

  std::cout << w.size() << "\n";
  std::cout << w.at<double>(0) << "\n";
  std::cout << w.at<int>(0) << "\n";

  std::cout << w0.size() << "\n";
  std::cout << w0.at<float>(0) << "\n";
  std::cout << w0.at<int>(0) << "\n";
  {
    auto & f = w0.at<het::hvector>(0).fraction<double>();
    for (auto i = 0ul; i < f.size(); ++i) {
      std::cout << f[i] << "\n";
    }
  }
  {
    auto & f = w0.at<het::hvector>(0).fraction<int>();
    for (auto i = 0ul; i < f.size(); ++i) {
      std::cout << f[i] << "\n";
    }
  }
  {
    auto & f = w0.at<het::hvector>(0).at<het::hvector>(0).fraction<double>();
    for (auto i = 0ul; i < f.size(); ++i) {
      std::cout << f[i] << "\n";
    }
  }
  {
    auto & f = w0.at<het::hvector>(0).at<het::hvector>(0).fraction<int>();
    for (auto i = 0ul; i < f.size(); ++i) {
      std::cout << f[i] << "\n";
    }
  }
}
