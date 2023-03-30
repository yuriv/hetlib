/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   error.h
 * Author: yuri
 *
 * Created on October 15, 2017, 2:16 PM
 */

#ifndef HETLIB_ERROR_H
#define HETLIB_ERROR_H

#include <system_error>
#include <errno.h>

namespace het {

/// \brief System error driver.
namespace sys {

enum class error_code : int {
  None,
  OutOfMemory,
  ResourceExhausted,
  EncodingError,
  BufferOverwrite,
  End
};

struct category : std::error_category {
  category() noexcept {}
  
  category(error_category const&) = delete;
  category(error_category&&) = delete;

  virtual ~category() noexcept {}

  category& operator=(error_category const&) = delete;
  category& operator=(error_category&&) = delete;

  char const* name() const noexcept override {
    return "system";
  }

  virtual std::string message(int c) const override {
    if(error_code(c) < error_code::End && error_code(c) >= error_code::None) {
      return _messages[c];
    }
    return "Unexpected error";
  }
  
private:
  constexpr static char const * _messages[int(error_code::End)] = {
    "No error",
    "Out of memory",
    "Resource exhausted",
    "Encoding error",
    "Buffer overwrite error"
  };
};

} // namespace sys

inline sys::category const & sys_error_cat() {
  static sys::category cat;
  return cat;
} 

inline std::error_code sys_error_code(sys::error_code c) {
  return std::error_code{int(c), sys_error_cat()};
}

/// \brief Control access error driver.
namespace access {

enum class error_code : int {
  None,
  UnboundedValue,
  ValueNotFound,
  End
};

struct category : std::error_category {
  category() noexcept {}
  
  category(error_category const &) = delete;
  category(error_category &&) = delete;

  virtual ~category() noexcept {}

  category& operator=(error_category const &) = delete;
  category& operator=(error_category &&) = delete;

  char const * name() const noexcept override {
    return "control access";
  }

  virtual std::string message(int c) const override {
    if(error_code(c) < error_code::End && error_code(c) >= error_code::None) {
      return _messages[c];
    }
    return "Unexpected error";
  }
  
private:
  constexpr static char const * _messages[int(error_code::End)] = {
    "No error",
    "Try to access unbounded value",
    "Value not found"
  };
};

} // namespace access

inline access::category const & access_error_cat() {
  static access::category cat;
  return cat;
} 

inline std::error_code access_error_code(access::error_code c) {
  return std::error_code{int(c), access_error_cat()};
}

} // namespace het

inline std::ostream & operator<<(std::ostream & os, std::error_code ec) {
  os << ec.message() << "(" << ec.value() << ")";
  return os;
}

#endif /* HETLIB_ERROR_H */
