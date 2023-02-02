//
// Created by yuri on 2/2/23.
//

#ifndef HETLIB_HETERO_TEST_H
#define HETLIB_HETERO_TEST_H

template <typename T> struct nvo {
  void print(std::ostream& os) const { os << _value << "\n"; }
  T _value;
  friend std::ostream& operator<<(std::ostream& os, nvo const& n) {
    n.print(os);
    return os;
  }
};

struct hetero_rec {
  het::hvector _h;
  std::pair<std::string_view, std::string> _p;
};

struct print_visitor {
  template<typename T> het::VisitorReturn operator()(T const & in) const {
    std::cout << in << " ";
    return het::VisitorReturn::Continue;
  }
};

inline std::ostream & operator<<(std::ostream & os, hetero_rec const & hr) {
  hr._h.template visit<int, double, char, std::string, nvo<float>>()(print_visitor{});
  os << "\n(" << hr._p.first << ", " << hr._p.second << ")\n";
  return os;
}

#endif //HETLIB_HETERO_TEST_H
