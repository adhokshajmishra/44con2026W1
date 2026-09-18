#pragma once

#include <iostream>
#include <string>

namespace teamtnt {

inline void log(const std::string& msg) {
  std::cout << "[teamtnt] " << msg << '\n';
}

inline void warn(const std::string& msg) {
  std::cerr << "[teamtnt][WARN] " << msg << '\n';
}

inline void die(const std::string& msg) {
  std::cerr << "[teamtnt][ERROR] " << msg << '\n';
  std::exit(1);
}

}  // namespace teamtnt
