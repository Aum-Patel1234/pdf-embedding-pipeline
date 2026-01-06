#include <faiss/IndexFlat.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string_view>
#include <thread>
#include <vector>

#include "../include/db.hpp"
#include "../include/init.hpp"
#include "../include/pdf_donwloader.hpp"
#include "../include/pdf_processor.hpp"
#include "../include/recursive_character_text_splitter.hpp"

int main() {
  loadenv("../.env");  // running from inside build/ dir
  const char* db_url = std::getenv("DATABASE_URL");
  if (!db_url) {
    std::cerr << "DATABASE_URL is absent.\n";
    return 1;
  }

  std::unique_ptr<pqxx::connection> conn = connectToDb(db_url);
  pqxx::work tx{*conn};

  const std::string topic = "natural language processing";

  int count = tx.exec_params("SELECT COUNT(*) FROM research_papers WHERE topic = $1", topic)[0][0].as<int>();
  // tx.commit();
  std::cout << "Count = " << count << "\n";

  std::vector<ResearchPaper> papers;
  papers.reserve(10);
  getPapersFromDb(tx, papers, topic, 0, 10);
  tx.commit();

  // Print all research papers
  for (const auto& paper : papers) {
    std::cout << "ID: " << paper.id << "\n";
    // std::cout << "Source: " << paperSourceToString(paper.source) << "\n";
    std::cout << "Source ID: " << paper.source_id << "\n";
    std::cout << "Title: " << paper.title << "\n";
    std::cout << "PDF URL: " << paper.pdf_url << "\n";

    std::cout << "Authors: ";
    for (size_t i = 0; i < paper.authors.size(); ++i) {
      std::cout << paper.authors[i];
      if (i != paper.authors.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";

    std::cout << "DOI: " << paper.doi << "\n";
    std::cout << "Embedding processed: " << (paper.embedding_processed ? "Yes" : "No") << "\n";
    std::cout << "Created at: " << paper.created_at << "\n";
    std::cout << "Topic: " << paper.topic << "\n";
    std::cout << "---------------------------\n";
  }
  // FAISS: declare a simple index
  faiss::IndexFlatL2 index(768);  // 768-dim embeddings

  // RecursiveCharacterTextSplitter r;
  // r.checkSplitter();
  //
  // try {
  //   const std::string_view url = "https://arxiv.org/pdf/1706.03762";
  //   const std::string_view file_path = "temp/temp_file1.pdf";
  //
  //   savePDFtoTextFile(url, file_path);
  //   std::string content = read_file(file_path);
  //   content = r.cleanText(content);
  //   std::vector<std::string_view> chunks = r.getChunks(content);
  //
  //   std::ofstream ofs("temp/temp_chunks.txt", std::ios::out);
  //   if (!ofs) {
  //     std::cerr << "Failed to open output file.\n";
  //     return 1;
  //   }
  //   for (size_t i = 0; i < chunks.size(); ++i) {
  //     const auto& chunk = chunks[i];
  //     ofs << "=== Chunk " << i << " (" << chunk.size() << " chars) ===\n";
  //     ofs.write(chunk.data(), chunk.size());
  //     ofs << "\n\n";
  //   }
  //   ofs.close();
  //
  // } catch (const std::exception& e) {
  //   std::cerr << "PDF download failed: " << e.what() << '\n';
  // }

  size_t thread = std::thread::hardware_concurrency();
  std::cout << "\nthreads = " << thread << "\n";

  return 0;
}
