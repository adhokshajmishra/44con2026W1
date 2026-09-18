#include "util.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace evasion {

namespace {

constexpr const char* kB64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64_index(char c) {
  const char* p = std::strchr(kB64, c);
  return p ? static_cast<int>(p - kB64) : -1;
}

}  // namespace

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

int run_command(const std::string& cmd) {
  return std::system(cmd.c_str());
}

std::string run_capture(const std::string& cmd) {
  std::array<char, 256> buffer{};
  std::string result;
  const std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) return result;
  while (fgets(buffer.data(), buffer.size(), pipe.get())) {
    result += buffer.data();
  }
  return result;
}

std::string base64_encode(const std::string& input) {
  std::string out;
  int val = 0;
  int valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(kB64[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) out.push_back(kB64[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4) out.push_back('=');
  return out;
}

std::string base64_decode(const std::string& input) {
  std::string out;
  std::vector<int> T(256, -1);
  for (int i = 0; i < 64; ++i) T[static_cast<unsigned char>(kB64[i])] = i;

  int val = 0;
  int valb = -8;
  for (unsigned char c : input) {
    if (c == '=') break;
    if (T[c] == -1) continue;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<char>((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

bool is_root() {
  return geteuid() == 0;
}

bool file_writable(const std::filesystem::path& path) {
  return access(path.c_str(), W_OK) == 0;
}

std::vector<std::string> read_proc_cmdlines() {
  std::vector<std::string> lines;
  for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
    if (!entry.is_directory()) continue;
    const auto name = entry.path().filename().string();
    if (name.empty() || name[0] < '0' || name[0] > '9') continue;
    const auto cmdline = entry.path() / "cmdline";
    std::ifstream in(cmdline, std::ios::binary);
    if (!in) continue;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    for (char& c : data) {
      if (c == '\0') c = ' ';
    }
    if (!data.empty()) lines.push_back(data);
  }
  return lines;
}

bool matches_agent_pattern(const std::string& text) {
  // TOOD: check for known names
  return false;
}

}  // namespace evasion
