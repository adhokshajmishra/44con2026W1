#include "config.hpp"

#include <cstdlib>
#include <cstring>

namespace evasion {

namespace {

std::string env_or(const char* key, const std::string& fallback) {
  const char* v = std::getenv(key);
  return v ? std::string(v) : fallback;
}

bool env_truthy(const char* key, bool fallback) {
  const char* v = std::getenv(key);
  if (!v) return fallback;
  return std::strcmp(v, "0") != 0 && std::strcmp(v, "false") != 0;
}

}  // namespace

Config Config::from_env() {
  Config c;
  c.lab_mode = env_truthy("TEAMTNT_EVASION_LAB", true);
  c.staging_dir = env_or("EVASION_STAGING", c.staging_dir);
  c.mock_miner_name = env_or("MOCK_MINER_NAME", c.mock_miner_name);
  c.mock_miner_path = env_or("MOCK_MINER_PATH", c.mock_miner_path);
  c.attack_staging = env_or("STAGING_DIR", c.attack_staging);
  c.preload_source = env_or("PRELOAD_SOURCE", c.preload_source);
  return c;
}

}  // namespace evasion
