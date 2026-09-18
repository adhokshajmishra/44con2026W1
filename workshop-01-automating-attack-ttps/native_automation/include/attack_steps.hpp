#pragma once

#include "config.hpp"
#include "docker_client.hpp"
#include "http_client.hpp"

#include <filesystem>

namespace teamtnt {

class AttackChain {
 public:
  explicit AttackChain(Config cfg);

  void run_all() const;
  void step01_scan() const;
  void step02_probe_docker() const;
  void step03_harvest_credentials() const;
  void step04_container_discovery() const;
  void step05_escape_and_deploy() const;
  void step06_lateral_movement() const;
  void step07_mock_miner() const;

 private:
  Config cfg_;
  HttpClient http_;
  DockerClient docker_;

  std::filesystem::path staging(const std::string& name) const;
};

}  // namespace teamtnt
