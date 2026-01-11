#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

constexpr std::string_view separators[] = {"\n\n", "\n", " ", ""};
constexpr uint16_t chunkSize = 1000;
constexpr uint8_t chunkOverlap = 200;
constexpr uint8_t lookahead = 200;
constexpr size_t MIN_CHUNK_CHARS = 500;  // don't embed chunks smaller than this

class RecursiveCharacterTextSplitter {
 private:
  std::vector<std::pair<size_t, size_t>> splitTextIntoChunks(std::string_view data) const;

 public:
  std::vector<std::string_view> getChunks(std::string_view data) const;
  std::string cleanText(std::string_view data);
};
