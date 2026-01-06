#include <string>
#include <vector>

#include "../models/document_model.hpp"
#include "db.hpp"

void embedding_pipeline(const char* DB_CONN_STR, const std::string& topic, uint32_t offset, uint32_t limit);
