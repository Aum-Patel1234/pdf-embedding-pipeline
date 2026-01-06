#pragma once

#include <pqxx/pqxx>

#include "../models/document_model.hpp"

std::unique_ptr<pqxx::connection> connectToDb(const char* DB_CONN_STR);

constexpr const char* GET_PAPERS_QUERY =
    "SELECT id, source, source_id, title, pdf_url, authors, doi, "
    "embedding_processed, created_at, topic "
    "FROM research_papers "
    "WHERE topic = $1 AND embedding_processed = false "
    "ORDER BY id "
    "LIMIT $2 OFFSET $3";

// NOTE: unique_ptr& to avoid taking ownership
void getPapersFromDb(pqxx::work& tx, std::vector<ResearchPaper>& papers, const std::string topic, const uint32_t offset,
                     const uint32_t limit);
// ResearchPaper papers[100]; // size = 100
// getPapersFromDb(papers, 0, 100, 100);
