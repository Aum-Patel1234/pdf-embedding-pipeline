#include <string>
#include <vector>

#include "../models/document_model.hpp"
#include "db.hpp"

// URL: https://ai.google.dev/gemini-api/docs/embeddings#rest
void getEmbeddings(const char* GEMINI_API_KEY, const char* model = "models/gemini-embedding-004");
void embedding_pipeline(const char* DB_CONN_STR, const std::string& topic, uint32_t offset, uint32_t limit);
