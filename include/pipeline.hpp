#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "../models/document_model.hpp"
#include "db.hpp"

void embedding_pipeline(const char* DB_CONN_STR, uint8_t thread_id, const std::string& topic, uint32_t offset,
                        uint32_t limit);
