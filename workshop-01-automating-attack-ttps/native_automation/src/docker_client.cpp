#include "docker_client.hpp"

#include "log.hpp"

#include <sstream>

namespace teamtnt {

DockerClient::DockerClient(const std::string& base_url) : base_(base_url) {}

bool DockerClient::ping() const {
  const auto resp = http_.get(base_ + "/_ping");
  return resp.status == 200;
}

nlohmann::json DockerClient::version() const {
  const auto resp = http_.get(base_ + "/version");
  if (!resp.ok()) return {};
  return nlohmann::json::parse(resp.body, nullptr, false);
}

nlohmann::json DockerClient::list_containers(bool all) const {
  std::string path = base_ + "/containers/json";
  if (all) path += "?all=1";
  const auto resp = http_.get(path);
  if (!resp.ok()) return nlohmann::json::array();
  return nlohmann::json::parse(resp.body, nullptr, false);
}

nlohmann::json DockerClient::list_images() const {
  const auto resp = http_.get(base_ + "/images/json");
  if (!resp.ok()) return nlohmann::json::array();
  return nlohmann::json::parse(resp.body, nullptr, false);
}

nlohmann::json DockerClient::inspect_container(const std::string& id) const {
  const auto resp = http_.get(base_ + "/containers/" + id + "/json");
  if (!resp.ok()) return {};
  return nlohmann::json::parse(resp.body, nullptr, false);
}

std::string DockerClient::find_container_by_name(const std::string& name) const {
  const auto containers = list_containers(true);
  if (!containers.is_array()) return {};
  for (const auto& c : containers) {
    if (!c.contains("Names")) continue;
    for (const auto& n : c["Names"]) {
      const std::string ns = n.get<std::string>();
      if (ns.find(name) != std::string::npos) {
        return c["Id"].get<std::string>();
      }
    }
  }
  return {};
}

std::string DockerClient::demux_stream(const std::string& raw) {
  std::string out;
  size_t i = 0;
  while (i + 8 <= raw.size()) {
    const uint8_t stream = static_cast<uint8_t>(raw[i]);
    const uint32_t size = (static_cast<uint8_t>(raw[i + 4]) << 24) |
                          (static_cast<uint8_t>(raw[i + 5]) << 16) |
                          (static_cast<uint8_t>(raw[i + 6]) << 8) |
                          static_cast<uint8_t>(raw[i + 7]);
    i += 8;
    if (i + size > raw.size()) break;
    if (stream == 1 || stream == 2) out.append(raw, i, size);
    i += size;
  }
  return out.empty() ? raw : out;
}

std::string DockerClient::exec_output(const std::string& container_id,
                                      const std::vector<std::string>& cmd) const {
  nlohmann::json body;
  body["AttachStdout"] = true;
  body["AttachStderr"] = true;
  body["Cmd"] = cmd;

  const auto create = http_.post(base_ + "/containers/" + container_id + "/exec",
                                 body.dump(), {{"Content-Type", "application/json"}});
  if (!create.ok()) return {};

  const auto j = nlohmann::json::parse(create.body, nullptr, false);
  if (!j.contains("Id")) return {};
  const std::string exec_id = j["Id"].get<std::string>();

  const auto start = http_.post(base_ + "/exec/" + exec_id + "/start",
                                R"({"Detach":false,"Tty":false})",
                                {{"Content-Type", "application/json"}});
  return demux_stream(start.body);
}

std::string DockerClient::create_container(const std::string& name,
                                           const nlohmann::json& spec) const {
  const std::string path = base_ + "/containers/create?name=" + name;
  const auto resp = http_.post(path, spec.dump(), {{"Content-Type", "application/json"}});
  if (!resp.ok()) {
    warn("create_container failed: " + resp.body);
    return {};
  }
  const auto j = nlohmann::json::parse(resp.body, nullptr, false);
  return j.contains("Id") ? j["Id"].get<std::string>() : std::string{};
}

void DockerClient::start_container(const std::string& id) const {
  http_.post_empty(base_ + "/containers/" + id + "/start",
                   {{"Content-Type", "application/json"}});
}

void DockerClient::remove_container(const std::string& name, bool force) const {
  std::string path = base_ + "/containers/" + name;
  if (force) path += "?force=1";
  http_.del(path);
}

void DockerClient::pull_image(const std::string& image, const std::string& tag) const {
  const std::string path = base_ + "/images/create?fromImage=" + image + "&tag=" + tag;
  http_.post_empty(path, {{"Content-Type", "application/json"}});
}

std::string DockerClient::run_container(const std::string& name, const nlohmann::json& spec) const {
  remove_container(name, true);
  const std::string id = create_container(name, spec);
  if (id.empty()) return {};
  start_container(id);
  return id;
}

}  // namespace teamtnt
