#include <faiss/IndexFlat.h>

#include <cstddef>
#include <iostream>
#include <pqxx/pqxx>
#include <string_view>
#include <thread>

#include "../include/db.hpp"
#include "../include/pdf_donwloader.hpp"
#include "../include/recursive_character_text_splitter.hpp"

int main() {
  // libpqxx: just declare a connection pointer (not connecting)
  pqxx::connection* conn = connectToDb();
  pqxx::work tx{*conn};

  int count = tx.query_value<int>("SELECT COUNT(*) FROM research_papers where topic='natural language processing'");
  std::cout << "Total research_papers = " << count << '\n';

  // FAISS: declare a simple index
  faiss::IndexFlatL2 index(768);  // 768-dim embeddings

  RecursiveCharacterTextSplitter r;
  r.checkSplitter();

  try {
    const std::string_view url = "https://arxiv.org/pdf/1706.03762";
    const std::string_view file_path = "temp/temp_file1.pdf";

    savePDFtoTextFile(url, file_path);
  } catch (const std::exception& e) {
    std::cerr << "PDF download failed: " << e.what() << '\n';
  }

  size_t thread = std::thread::hardware_concurrency();
  std::cout << "\nthreads = " << thread << "\n";

  std::cout << "Build successful: libpqxx + FAISS linked\n";
  return 0;
}
