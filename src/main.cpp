#include <faiss/IndexFlat.h>
// #include <faiss/IndexIVFFlat.h>
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h>

#include <array>
#include <cstdint>
#include <pqxx/pqxx>
#include <thread>
#include <vector>

#include "../include/consts.hpp"
#include "../include/init.hpp"
#include "../include/logger.hpp"
#include "../include/pipeline.hpp"

int main() {
  loadenv();
  const char* db_url = std::getenv("DATABASE_URL");
  if (!db_url) {
    logging::log_error("DATABASE_URL is absent.");
    return 1;
  }

  const std::string topic = std::getenv("TOPIC");
  const std::string BATCH_URL = std::getenv("BATCH_URL");
  const std::string MODEL_NAME = std::getenv("MODEL_NAME");
  uint32_t total_papers = 5120;
  uint32_t processed = 0;

  // merge faiss indexes
  faiss::IndexFlatIP base_index(OUTPUT_DIM);
  faiss::IndexIDMap final_index(&base_index);

  while (processed < total_papers) {
    // std::array<uint32_t, THREADS> LIMITS = getLimits(THREADS, get_total_papers_topic(topic, db_url));
    std::array<uint32_t, THREADS> LIMITS = {128, 128, 128, 128};
    uint32_t batch_offset = processed;  //

    std::vector<std::thread> threads;
    std::filesystem::create_directories("faiss");
    threads.reserve(THREADS);
    for (uint8_t i = 0; i < THREADS; ++i) {
      uint32_t batch_size = std::min(LIMITS[i], total_papers - batch_offset);
      if (batch_size == 0) break;

      threads.emplace_back(embedding_pipeline, db_url, BATCH_URL, MODEL_NAME, i, topic, batch_offset, batch_size);
      batch_offset += batch_size;
    }

    // wait for all threads
    for (auto& t : threads) t.join();

    // NOTE: while retriving or searching IndexIVFFlat does do good in RAM - CPU
    // faiss::IndexIVFFlat index(&final_index, OUTPUT_DIM, NLIST, faiss::METRIC_INNER_PRODUCT);
    for (uint8_t i = 0; i < THREADS; ++i) {
      std::string path = "faiss/papers_" + std::to_string(i) + ".faiss";
      if (!std::filesystem::exists(path)) {
        logging::log_error("Shard missing, skipping: " + path);
        continue;
      }
      std::unique_ptr<faiss::Index> shard(faiss::read_index(path.c_str()));
      faiss::IndexIDMap* shard_idmap = dynamic_cast<faiss::IndexIDMap*>(shard.get());

      if (!shard_idmap) {
        logging::log_error("Shard index is not IndexIDMap: " + path);
        throw std::runtime_error("Shard index is not IndexIDMap: " + path);
      }

      // std::vector<float> buffer(shard_idmap->ntotal * OUTPUT_DIM);
      // std::vector<faiss::idx_t> ids(shard_idmap->ntotal);
      // for (faiss::idx_t j = 0; j < shard_idmap->ntotal; ++j) {
      //   shard_idmap->reconstruct(j, buffer.data() + j * OUTPUT_DIM);
      //   ids[j] = shard_idmap->id_map[j];  // PRESERVE ID
      // }
      //
      // // Add WITH IDs
      // final_index.add_with_ids(shard_idmap->ntotal, buffer.data(), ids.data());
      final_index.merge_from(*shard_idmap);
      std::filesystem::remove(path);
    }

    faiss::write_index(&final_index, "paper.faiss");
    logging::log_checkpoint("Batch saved: papers " + std::to_string(processed) + " to " +
                            std::to_string(batch_offset - 1));

    processed = batch_offset;
  }

  return 0;
}
