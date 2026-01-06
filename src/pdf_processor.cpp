#include "../include/pdf_processor.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

#include "poppler/cpp/poppler-document.h"
#include "poppler/cpp/poppler-page.h"

namespace fs = std::filesystem;

// IMPORTANT: https://stackoverflow.com/questions/2732178/extracting-text-from-pdf-with-poppler-c
std::string read_file(std::string_view file_path) {
  if (!fs::exists(file_path)) {
    std::cout << "File path " << file_path << " does not exist to read it.";
    return "";
  }

  poppler::document* doc = poppler::document::load_from_file(file_path.data());
  if (!doc) {
    std::cerr << "Failed to open PDF: " << file_path << "\n";
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
