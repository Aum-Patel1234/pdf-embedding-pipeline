#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

struct EmbeddingChunk {
  // ---- Primary key ----
  int64_t id;           // BIGSERIAL (db-generated)
  int64_t faiss_id;     // UNIQUE, maps directly to FAISS vector ID
  int64_t document_id;  // FK → research_papers.id
  int32_t chunk_index;  // order inside the document
  std::optional<uint32_t> page_number;
  std::string chunk_text;
  std::string embedding_model;
  // std::chrono::system_clock::time_point created_at;
};
