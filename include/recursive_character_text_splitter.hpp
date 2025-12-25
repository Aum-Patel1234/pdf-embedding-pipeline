#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

constexpr std::string_view separators[] = {"\n\n", "\n", " ", ""};
constexpr uint16_t chunkSize = 1000;
constexpr uint8_t chunkOverlap = 200;

class RecursiveCharacterTextSplitter {
 private:
  std::vector<std::pair<size_t, size_t>> splitTextIntoChunks(std::string_view data) const;

 public:
  std::vector<std::string_view> getChunks(std::string_view data) const;
  std::string cleanText(std::string_view data);
  void checkSplitter() const;
};
