#pragma once

#include <iostream>
#include <string>

namespace evasion {

inline void log(const std::string& msg) {
  std::cout << "[evasion] " << msg << '\n';
}

inline void warn(const std::string& msg) {
  std::cerr << "[evasion][WARN] " << msg << '\n';
}

inline void die(const std::string& msg) {
  std::cerr << "[evasion][ERROR] " << msg << '\n';
  std::exit(1);
}

}  // namespace evasion
