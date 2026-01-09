#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>

#include <cstdint>
#include <pqxx/pqxx>
#include <thread>
#include <vector>

#include "../include/consts.hpp"
#include "../include/init.hpp"
#include "../include/logger.hpp"
#include "../include/pipeline.hpp"

int main() {
  loadenv("../.env");  // running from inside build/ dir
  const char* db_url = std::getenv("DATABASE_URL");
  if (!db_url) {
    logging::log_error("DATABASE_URL is absent.");
    return 1;
  }

  const std::string topic = "natural language processing";
  uint32_t offset = 0;

  std::vector<std::thread> threads;
  std::filesystem::create_directories("faiss");
  threads.reserve(THREADS);
  for (uint8_t i = 0; i < THREADS; ++i) {
    threads.emplace_back(embedding_pipeline, db_url, i, topic, offset, LIMITS[i]);
    offset += LIMITS[i];
  }

  // wait for all threads
  for (auto& t : threads) {
    t.join();
  }

  // merge faiss indexes
  faiss::IndexFlatIP final_index(OUTPUT_DIM);
  for (uint8_t i = 0; i < THREADS; ++i) {
    std::string path = "faiss/papers_" + std::to_string(i) + ".faiss";
    if (!std::filesystem::exists(path)) {
      logging::log_error("Shard missing, skipping: " + path);
      continue;
    }
    faiss::Index* shard = faiss::read_index(path.c_str());

    std::vector<float> buffer(shard->ntotal * OUTPUT_DIM);
    for (faiss::idx_t j = 0; j < shard->ntotal; ++j) {
      shard->reconstruct(j, buffer.data() + j * OUTPUT_DIM);
    }

    final_index.add(shard->ntotal, buffer.data());
    delete shard;
  }

  faiss::write_index(&final_index, "paper.faiss");

  return 0;
}
