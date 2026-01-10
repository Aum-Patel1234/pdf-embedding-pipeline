#pragma once

#include <filesystem>
#include <string_view>
#include <vector>
// std::string read_file(std::string_view file_path);
std::vector<std::pair<uint32_t, std::string>> read_file(std::string_view file_path);
