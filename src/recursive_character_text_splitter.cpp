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

void RecursiveCharacterTextSplitter::checkSplitter() const {
  RecursiveCharacterTextSplitter splitter;

  // ------------------------------
  // Test Case 1: Long single paragraph
  // ------------------------------
  std::string text1 =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
      "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
      "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. "
      "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
      "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

  auto chunks1 = splitter.getChunks(text1);
  std::cout << "Test Case 1: " << chunks1.size() << " chunks\n";
  for (size_t i = 0; i < chunks1.size(); ++i) {
    std::cout << "Chunk " << i << " (" << chunks1[i].size() << " chars):\n";
    std::cout << chunks1[i] << "\n\n";
  }

  // ------------------------------
  // Test Case 2: Multiple paragraphs with newlines
  // ------------------------------
  std::string text2 =
      "Paragraph one starts here and continues with enough text to exceed the chunk size. "
      "We need to make sure that the splitter correctly handles the overlap as well.\n\n"
      "Paragraph two is here. It also has a lot of words to fill the chunk size and test the overlap feature properly. "
      "The text should split without cutting off in the middle of words unnecessarily.\n\n"
      "Paragraph three is the final one to check that everything works as expected and that the last chunk is handled "
      "correctly.";

  auto chunks2 = splitter.getChunks(text2);
  std::cout << "Test Case 2: " << chunks2.size() << " chunks\n";

  // ------------------------------
  // Test Case 3: Extremely long paragraph (simulate PDF)
  // ------------------------------
  std::string text3;
  for (int i = 0; i < 20; ++i) {  // repeat 20 times to make it long
    text3 +=
        "This is a line from a simulated PDF file containing multiple sentences and paragraphs. "
        "It is designed to exceed the chunk size and ensure overlap is applied correctly. ";
  }

  auto chunks3 = splitter.getChunks(text3);
  std::cout << "Test Case 3: " << chunks3.size() << " chunks\n";
}
