#include "../include/pdf_processor.hpp"

#include "../include/logger.hpp"
#include "poppler/cpp/poppler-document.h"
#include "poppler/cpp/poppler-page.h"

namespace fs = std::filesystem;

// IMPORTANT: https://stackoverflow.com/questions/2732178/extracting-text-from-pdf-with-poppler-c
std::vector<std::pair<uint32_t, std::string>> read_file(std::string_view file_path) {
  std::vector<std::pair<uint32_t, std::string>> pages_text;

  if (!fs::exists(file_path)) {
    logging::log_error("File path " + std::string(file_path) + " does not exist to read it.");
    return pages_text;
  }

  std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(file_path.data()));
  if (!doc) {
    logging::log_error("Failed to open PDF: " + std::string(file_path));
    return pages_text;
  }

  int pagesNbr = doc->pages();
  for (int i = 0; i < pagesNbr; i++) {
    std::unique_ptr<poppler::page> page(doc->create_page(i));
    if (!page) continue;

    std::string page_text = page->text().to_latin1();
    pages_text.emplace_back(i + 1, page_text);  // store 1-based page number
  }

  return pages_text;
}
