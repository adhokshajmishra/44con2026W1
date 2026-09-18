#pragma once

#include "http_client.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace teamtnt {

class DockerClient {
 public:
  explicit DockerClient(const std::string& base_url);

  bool ping() const;
  nlohmann::json version() const;
  nlohmann::json list_containers(bool all = false) const;
  nlohmann::json list_images() const;
  nlohmann::json inspect_container(const std::string& id) const;
  std::string find_container_by_name(const std::string& name) const;
  std::string exec_output(const std::string& container_id,
                          const std::vector<std::string>& cmd) const;
  std::string create_container(const std::string& name, const nlohmann::json& spec) const;
  void start_container(const std::string& id) const;
  void remove_container(const std::string& name, bool force = true) const;
  void pull_image(const std::string& image, const std::string& tag) const;
  std::string run_container(const std::string& name, const nlohmann::json& spec) const;

 private:
  std::string base_;
  HttpClient http_;

  static std::string demux_stream(const std::string& raw);
};

}  // namespace teamtnt
