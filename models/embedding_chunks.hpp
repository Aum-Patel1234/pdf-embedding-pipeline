#pragma once

#include <faiss/MetricType.h>

#include <cstdint>
#include <string>

struct EmbeddingChunk {
  // ---- Primary key ----
  int64_t id;           // BIGSERIAL (db-generated) NOTE: this is also the faiss_id
  int64_t document_id;  // FK → research_papers.id
  int32_t chunk_index;  // order inside the document
  uint32_t page_number;
  std::string chunk_text;
  std::string embedding_model;
  // std::chrono::system_clock::time_point created_at;
};
