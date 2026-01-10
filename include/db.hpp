#pragma once

#include <pqxx/pqxx>

#include "../include/queries.hpp"
#include "../models/document_model.hpp"
#include "../models/embedding_chunks.hpp"
#include "embedding_vector.hpp"

std::unique_ptr<pqxx::connection> connectToDb(const char* DB_CONN_STR);
void getPapersFromDb(pqxx::work& tx, std::vector<ResearchPaper>& papers, const std::string topic, const uint32_t offset,
                     const uint32_t limit);
void insert_embedding_chunk(pqxx::work& tx, const EmbeddingChunk& chunk);
void insert_embedding_vector(pqxx::work& tx, const EmbeddingVector& ev);
