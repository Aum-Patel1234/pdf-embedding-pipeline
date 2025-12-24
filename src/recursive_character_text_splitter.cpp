#include "../include/recursive_character_text_splitter.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

std::vector<std::string_view> RecursiveCharacterTextSplitter::splitTextIntoChunks(const std::string_view data) {
  std::vector<std::string_view> chunks;
  uint8_t idx = 0;

  for (uint8_t i = 0; i < separators->size(); ++i) {
    if (separators[i].empty()) {
      idx = i;
      break;
    }
    if (data.find(separators[i]) != std::string_view::npos) {
      idx = i;
      break;
    }
  }

  const std::string_view sep = separators[idx];

  return chunks;
}
