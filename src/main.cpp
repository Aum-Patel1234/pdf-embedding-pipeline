#include <faiss/IndexFlat.h>

#include <cstddef>
#include <cstdint>
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
#include "../include/pipeline.hpp"
#include "../include/recursive_character_text_splitter.hpp"

// constexpr int TOTAL_PAPERS = 80649;  // i have in database
constexpr uint8_t THREADS = 6;
// constexpr uint32_t LIMITS[6] = {13442, 13442, 13442, 13441, 13441, 13441};
constexpr uint32_t LIMITS[6] = {20, 20, 20, 20, 20, 20};

int main() {
  loadenv("../.env");  // running from inside build/ dir
  const char* db_url = std::getenv("DATABASE_URL");
  if (!db_url) {
    std::cerr << "DATABASE_URL is absent.\n";
    return 1;
  }

  const std::string topic = "natural language processing";
  uint32_t offset = 0;

  std::vector<std::thread> threads;
  threads.reserve(THREADS);
  for (uint8_t i = 0; i < THREADS; ++i) {
    threads.emplace_back(embedding_pipeline, db_url, topic, offset, LIMITS[i]);
    offset += LIMITS[i];
  }

  // wait for all threads
  for (auto& t : threads) {
    t.join();
  }

  // std::unique_ptr<pqxx::connection> conn = connectToDb(db_url);
  // pqxx::work tx{*conn};

  // int count = tx.exec_params("SELECT COUNT(*) FROM research_papers WHERE topic = $1", topic)[0][0].as<int>();
  // // tx.commit();
  // std::cout << "Count = " << count << "\n";

  // std::vector<ResearchPaper> papers;
  // papers.reserve(10);
  // getPapersFromDb(tx, papers, topic, 0, 10);
  // tx.commit();

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

  return 0;
}
