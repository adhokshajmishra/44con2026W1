#include "net_scanner.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>

namespace teamtnt {

bool tcp_connect(const std::string& host, int port, int timeout_ms) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* res = nullptr;
  const std::string port_str = std::to_string(port);
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
    return false;
  }

  bool connected = false;
  for (addrinfo* p = res; p; p = p->ai_next) {
    // TODO: perform connect scan here
  }

  freeaddrinfo(res);
  return connected;
}

std::vector<ScanResult> scan_targets(const std::vector<ScanTarget>& targets, int timeout_ms) {
  std::vector<ScanResult> results;
  for (const auto& t : targets) {
    for (int port : t.ports) {
      if (tcp_connect(t.host, port, timeout_ms)) {
        ScanResult r;
        r.host = t.host;
        r.port = port;
        char ip[INET6_ADDRSTRLEN]{};
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(t.host.c_str(), nullptr, &hints, &res) == 0 && res) {
          if (res->ai_family == AF_INET) {
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr, ip,
                      sizeof(ip));
          } else if (res->ai_family == AF_INET6) {
            inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(res->ai_addr)->sin6_addr, ip,
                      sizeof(ip));
          }
          freeaddrinfo(res);
        }
        r.ip = ip;
        results.push_back(r);
      }
    }
  }
  return results;
}

}  // namespace teamtnt
