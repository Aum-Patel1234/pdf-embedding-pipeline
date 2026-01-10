#pragma once

#include <pqxx/pqxx>

#include "../include/queries.hpp"
#include "../models/document_model.hpp"
#include "../models/embedding_chunks.hpp"

std::unique_ptr<pqxx::connection> connectToDb(const char* DB_CONN_STR);
void getPapersFromDb(pqxx::work& tx, std::vector<ResearchPaper>& papers, const std::string topic, const uint32_t offset,
                     const uint32_t limit);
void insert_embedding_chunk(pqxx::work& txn, const EmbeddingChunk& chunk);
