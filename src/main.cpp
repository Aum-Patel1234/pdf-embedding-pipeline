#include <faiss/IndexFlat.h>

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
    log_error("DATABASE_URL is absent.");
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

  return 0;
}
