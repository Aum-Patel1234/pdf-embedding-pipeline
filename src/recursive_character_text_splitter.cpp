#include "../include/recursive_character_text_splitter.hpp"

#include <cstddef>

std::string RecursiveCharacterTextSplitter::cleanText(std::string_view data) {
  std::string result;
  result.reserve(data.size());

  bool in_space = false;
  for (char c : data) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!in_space) {
        result += ' ';  // replace all whitespace sequences with a single space
        in_space = true;
      }
    } else if (std::isprint(static_cast<unsigned char>(c))) {
      result += c;
      in_space = false;
    }
    // else skip non-printable characters
  }

  return result;
}

std::vector<std::pair<size_t, size_t>> RecursiveCharacterTextSplitter::splitTextIntoChunks(
    std::string_view data) const {
  std::vector<std::pair<size_t, size_t>> ranges;

  uint8_t idx = separators->size() - 1;
  for (size_t i = 0; i < separators->size(); ++i) {
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
  size_t start = 0;

  while (start < data.size()) {
    size_t maxEnd = std::min(start + chunkSize, data.size());
    size_t searchEnd = std::min(maxEnd + lookahead, data.size());

    if (sep.empty()) {
      ranges.emplace_back(start, maxEnd);
      start = maxEnd;
      continue;
    }

    size_t it = data.find(sep, start);

    if (it == std::string_view::npos || it > searchEnd) {
      // no separator inside the current window + lookahead -> take up to maxEnd
      ranges.emplace_back(start, maxEnd);
      start = maxEnd;
    } else {
      // separator found inside window -> split at separator start
      ranges.emplace_back(start, it + sep.size());
      start = it + sep.size();
    }
  }

  return ranges;
}

std::vector<std::string_view> RecursiveCharacterTextSplitter::getChunks(std::string_view data) const {
  const auto ranges = splitTextIntoChunks(data);
  std::vector<std::string_view> finalChunks;
  finalChunks.reserve(ranges.size());

  if (ranges.empty()) return finalChunks;

  size_t s = ranges[0].first;
  size_t e = ranges[0].second;
  finalChunks.emplace_back(data.substr(s, e - s));

  for (size_t i = 1; i < ranges.size(); ++i) {
    s = ranges[i].first;
    e = ranges[i].second;

    // apply overlap with previous chunk by expanding start backwards
    size_t prev_len = ranges[i - 1].second - ranges[i - 1].first;
    size_t overlap = std::min(static_cast<size_t>(chunkOverlap), prev_len);

    s = (s >= overlap) ? (s - overlap) : 0;

    while (s < e && !std::isspace(static_cast<unsigned char>(data[s]))) {
      ++s;
    }
    // skip the whitespace itself
    while (s < e && std::isspace(static_cast<unsigned char>(data[s]))) {
      ++s;
    }

    finalChunks.emplace_back(data.substr(s, e - s));
  }

  return finalChunks;
}
