#pragma once

#include <array>
#include <cstdint>
#include <sstream>

#include "consts.hpp"

struct EmbeddingVector {
  int64_t id;                  // BIGSERIAL
  int64_t embedding_chunk_id;  // FK → embedding_chunks.id
  std::array<float, OUTPUT_DIM> embedding;
  // OR use std::vector<float> if you prefer
  // std::vector<float> embedding;
  // ---- Metadata ----
  // std::chrono::system_clock::time_point created_at;
};

// NOTE: pg_vector expects - '[0.12, 0.53, -0.88, ...]'
template <size_t N>
std::string to_pgvector(const std::array<float, N>& embedding) {
  std::ostringstream oss;
  oss << "[";

  for (size_t i = 0; i < N; ++i) {
    oss << embedding[i];
    if (i + 1 < N) oss << ",";
  }

  oss << "]";
  return oss.str();
}
