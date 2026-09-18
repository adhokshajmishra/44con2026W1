#pragma once

#include <string>
#include <vector>

namespace teamtnt {

struct ScanTarget {
  std::string host;
  std::vector<int> ports;
};

struct ScanResult {
  std::string host;
  std::string ip;
  int port{0};
};

std::vector<ScanResult> scan_targets(const std::vector<ScanTarget>& targets, int timeout_ms = 2000);
bool tcp_connect(const std::string& host, int port, int timeout_ms);

}  // namespace teamtnt
