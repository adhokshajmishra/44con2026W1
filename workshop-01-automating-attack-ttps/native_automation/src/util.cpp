#include "util.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace teamtnt {

void ensure_dir(const std::filesystem::path& path) {
  std::filesystem::create_directories(path);
}

void write_file(const std::filesystem::path& path, const std::string& content) {
  ensure_dir(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  out << content;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void append_file(const std::filesystem::path& path, const std::string& content) {
  ensure_dir(path.parent_path());
  std::ofstream out(path, std::ios::app);
  out << content;
}

std::string extract_ssh_key(const std::string& text) {
  const auto begin = text.find("-----BEGIN OPENSSH PRIVATE KEY-----");
  const auto end = text.find("-----END OPENSSH PRIVATE KEY-----");
  if (begin == std::string::npos || end == std::string::npos) return {};
  const std::string end_marker = "-----END OPENSSH PRIVATE KEY-----";
  std::size_t out_end = end + end_marker.size();
  if (out_end < text.size() && text[out_end] == '\n') out_end++;
  return text.substr(begin, out_end - begin);
}

std::string extract_ssh_password(const std::string& text) {
  const std::string prefix = "devops:";
  const auto pos = text.find(prefix);
  if (pos == std::string::npos) return {};
  const auto start = pos + prefix.size();
  const auto end = text.find('\n', start);
  if (end == std::string::npos) return text.substr(start);
  return text.substr(start, end - start);
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    if (!item.empty()) parts.push_back(item);
  }
  return parts;
}

int run_command(const std::string& cmd) {
  return std::system(cmd.c_str());
}

}  // namespace teamtnt
