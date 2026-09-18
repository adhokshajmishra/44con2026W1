#pragma once

#include <string>
#include <vector>

namespace teamtnt {

struct ScanEndpoint {
  std::string label;
  std::string host;
  std::vector<int> ports;
};

struct Config {
  bool lab_mode{true};
  std::string staging_dir{"/tmp/teamtnt-staging"};
  std::string docker_host{"docker-api"};
  int docker_port{2375};
  std::string metadata_url{"http://metadata-sim/latest/meta-data"};
  std::string ssh_lateral_target{"lateral-ssh-target"};
  std::vector<ScanEndpoint> scan_targets;
  std::string profile{"in-lab"};  // "in-lab" or "host"

  static Config from_env();
  std::string docker_base_url() const;
};

}  // namespace teamtnt
