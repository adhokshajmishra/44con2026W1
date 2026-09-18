#include "attack_steps.hpp"
#include "config.hpp"
#include "log.hpp"

#include <curl/curl.h>
#include <cstring>
#include <iostream>

namespace {

void print_usage() {
  std::cout << "Usage: teamtnt_native [step]\n"
            << "  step: 1-7 or 'all' (default)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  const auto cfg = teamtnt::Config::from_env();
  teamtnt::AttackChain chain(cfg);

  std::string mode = "all";
  if (argc > 1) mode = argv[1];

  try {
    if (mode == "all") {
      chain.run_all();
    } else if (mode == "1") {
      chain.step01_scan();
    } else if (mode == "2") {
      chain.step02_probe_docker();
    } else if (mode == "3") {
      chain.step03_harvest_credentials();
    } else if (mode == "4") {
      chain.step04_container_discovery();
    } else if (mode == "5") {
      chain.step05_escape_and_deploy();
    } else if (mode == "6") {
      chain.step06_lateral_movement();
    } else if (mode == "7") {
      chain.step07_mock_miner();
    } else {
      print_usage();
      curl_global_cleanup();
      return 1;
    }
  } catch (const std::exception& ex) {
    teamtnt::die(std::string("exception: ") + ex.what());
  }

  curl_global_cleanup();
  return 0;
}
