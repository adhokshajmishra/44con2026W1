#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace evasion {

void ensure_dir(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::string& content);
std::string read_file(const std::filesystem::path& path);
void append_file(const std::filesystem::path& path, const std::string& content);
int run_command(const std::string& cmd);
std::string run_capture(const std::string& cmd);
std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);
bool is_root();
bool file_writable(const std::filesystem::path& path);
std::vector<std::string> read_proc_cmdlines();
bool matches_agent_pattern(const std::string& text);

}  // namespace evasion
