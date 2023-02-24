//
// Created by yuri on 2/12/23.
//

#include "het/het.h"

#include <iostream>

using namespace std::string_view_literals;
using namespace std::string_literals;

int main() {
  het::hvector{""sv, std::make_tuple(1, 2.f), ""s, 1.}
    .match<std::string_view/*exactly*/, std::tuple<int, float>/*exactly*/, double/*default*/, std::string/*string_view cast*/, int*/*absent*/>()(
      [](std::tuple<int, float>){std::cout << 1;},
      [](std::string_view){std::cout << 2;},
      [](auto){std::cout << "default";} // should be last
    );

  return 0;
}
