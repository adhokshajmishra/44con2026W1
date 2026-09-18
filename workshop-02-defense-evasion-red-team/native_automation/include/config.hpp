#pragma once

#include <string>

namespace evasion {

struct Config {
  bool lab_mode{true};
  std::string staging_dir{"/tmp/teamtnt-evasion"};
  std::string mock_miner_name{"k8s-guardian"};
  std::string mock_miner_path{"/tmp/.dockerd"};
  std::string attack_staging{"/tmp/teamtnt-staging"};
  std::string preload_source{"/opt/teamtnt/evasion/lib/libprocesshider-demo.c"};

  static Config from_env();
};

}  // namespace evasion
