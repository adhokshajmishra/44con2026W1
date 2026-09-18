#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace teamtnt {

void ensure_dir(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::string& content);
std::string read_file(const std::filesystem::path& path);
void append_file(const std::filesystem::path& path, const std::string& content);
std::string extract_ssh_key(const std::string& text);
std::string extract_ssh_password(const std::string& text);
std::vector<std::string> split(const std::string& s, char delim);
int run_command(const std::string& cmd);

}  // namespace teamtnt
