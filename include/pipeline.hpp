#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../models/document_model.hpp"
#include "db.hpp"

void embedding_pipeline(const char* DB_CONN_STR, uint8_t thread_id, const std::string& topic, uint32_t offset,
                        uint32_t limit);

std::vector<float> get_embeddings_from_embedding_model(const std::vector<std::string_view>& chunks,
                                                       const ResearchPaper& researchPaper);

void write_to_temp_file_txt_to_debug(const std::vector<std::string_view>& chunks, const std::string& chunks_file);
