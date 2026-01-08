#include "../include/pdf_processor.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

#include "../include/logger.hpp"
#include "poppler/cpp/poppler-document.h"
#include "poppler/cpp/poppler-page.h"

namespace fs = std::filesystem;

// IMPORTANT: https://stackoverflow.com/questions/2732178/extracting-text-from-pdf-with-poppler-c
std::string read_file(std::string_view file_path) {
  if (!fs::exists(file_path)) {
    logging::log_error("File path " + std::string(file_path) + " does not exist to read it.");
    return "";
  }

  poppler::document* doc = poppler::document::load_from_file(file_path.data());
  if (!doc) {
    logging::log_error("Failed to open PDF: " + std::string(file_path));
    return {};
  }
  int pagesNbr = doc->pages();
  // std::cout << "Page count: " << pagesNbr << "\n";

  std::string str;
  for (int i = 0; i < pagesNbr; i++) {
    str += doc->create_page(i)->text().to_latin1().c_str();
  }

  return str;
}
