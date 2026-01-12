#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "consts.hpp"
#include "db.hpp"
#include "faiss_store.hpp"
#include "logger.hpp"
#include "recursive_character_text_splitter.hpp"

void embedding_pipeline(const char* DB_CONN_STR, std::string BATCH_URL, std::string MODEL_NAME, uint8_t thread_id,
                        const std::string& topic, uint32_t offset, uint32_t limit);

std::pair<std::vector<std::vector<float>>, std::vector<float>> get_embeddings_from_embedding_model(
    std::string& MODEL_NAME, std::string& BATCH_URL, const std::vector<std::string_view>& chunks);

void write_to_temp_file_txt_to_debug(const std::vector<std::string_view>& chunks, const std::string& chunks_file);
uint32_t get_total_papers_topic(std::string topic, const char* db_url);

// NOTE: helper funcs
inline std::vector<std::string_view> getMeaningfulChunks(const RecursiveCharacterTextSplitter& splitter,
                                                         const std::string& text, size_t min_size = MIN_CHUNK_CHARS) {
  std::vector<std::string_view> chunks = splitter.getChunks(text);

  std::vector<std::string_view> meaningful_chunks;
  meaningful_chunks.reserve(chunks.size());

  for (auto& chunk : chunks) {
    if (chunk.size() >= min_size) {
      meaningful_chunks.push_back(chunk);
    }
  }

  return meaningful_chunks;
}

inline std::vector<faiss::idx_t> storeChunksAndGetIds(pqxx::work& tx,
                                                      const std::vector<std::string_view>& meaningful_chunks,
                                                      int64_t document_id, uint32_t page_no, uint32_t& chunk_index,
                                                      const std::string& model_name) {
  std::vector<EmbeddingChunk> embedding_chunks;
  embedding_chunks.reserve(meaningful_chunks.size());

  for (size_t i = 0; i < meaningful_chunks.size(); ++i) {
    EmbeddingChunk emb_chunk;
    emb_chunk.chunk_index = chunk_index++;
    emb_chunk.page_number = page_no;
    emb_chunk.document_id = document_id;
    emb_chunk.embedding_model = model_name;
    emb_chunk.chunk_text = meaningful_chunks[i];

    embedding_chunks.emplace_back(std::move(emb_chunk));
  }

  // Insert into Postgres and return the IDs
  return insert_embedding_chunks(tx, embedding_chunks);
}

inline std::vector<EmbeddingVector> make_embedding_vectors(const std::vector<std::vector<float>>& vec_of_embeddings,
                                                           const std::vector<faiss::idx_t>& chunk_ids) {
  if (vec_of_embeddings.size() != chunk_ids.size()) {
    logging::log_error("Embedding vectors size does not match chunk IDs size!");
    return {};
  }

  std::vector<EmbeddingVector> embedding_vectors;
  embedding_vectors.reserve(vec_of_embeddings.size());

  for (size_t i = 0; i < vec_of_embeddings.size(); ++i) {
    const auto& vec = vec_of_embeddings[i];

    if (vec.size() != OUTPUT_DIM) {
      logging::log_error("Embedding dimension mismatch at index " + std::to_string(i));
      continue;
    }

    EmbeddingVector ev;
    ev.embedding_chunk_id = chunk_ids[i];
    // Copy vector into std::array
    std::copy_n(vec.begin(), OUTPUT_DIM, ev.embedding.begin());
    embedding_vectors.push_back(std::move(ev));
  }

  return embedding_vectors;
}
