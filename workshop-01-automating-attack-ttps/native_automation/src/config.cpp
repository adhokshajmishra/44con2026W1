#include "config.hpp"

#include <cstdlib>
#include <cstring>

namespace teamtnt {

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

void parse_docker_url(const std::string& url, std::string& host, int& port) {
  auto s = url;
  if (s.rfind("tcp://", 0) == 0) s = s.substr(6);
  const auto colon = s.rfind(':');
  if (colon != std::string::npos) {
    host = s.substr(0, colon);
    port = std::stoi(s.substr(colon + 1));
  } else {
    host = s;
    port = 2375;
  }
}

}  // namespace

Config Config::from_env() {
  Config c;
  c.lab_mode = env_truthy("TEAMTNT_LAB", true);
  c.staging_dir = env_or("STAGING_DIR", c.staging_dir);
  c.metadata_url = env_or("METADATA_URL", c.metadata_url);

  const auto docker_url = env_or("DOCKER_API_URL", "tcp://docker-api:2375");
  parse_docker_url(docker_url, c.docker_host, c.docker_port);
  c.ssh_lateral_target = env_or("SSH_LATERAL_TARGET", c.ssh_lateral_target);
  return c;
}

std::string Config::docker_base_url() const {
  return "http://" + docker_host + ":" + std::to_string(docker_port);
}

}  // namespace teamtnt
