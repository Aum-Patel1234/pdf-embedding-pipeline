#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

constexpr std::string_view separators[] = {"\n\n", "\n", " ", ""};
constexpr uint16_t chunkSize = 100;
constexpr uint8_t chunkOverlap = 20;

class RecursiveCharacterTextSplitter {
 private:
 public:
  std::vector<std::string_view> splitTextIntoChunks(std::string_view data);
};
