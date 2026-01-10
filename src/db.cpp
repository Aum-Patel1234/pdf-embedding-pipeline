#include "../include/db.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <pqxx/internal/concat.hxx>
#include <vector>

std::unique_ptr<pqxx::connection> connectToDb(const char* DB_CONN_STR) {
  auto conn = std::make_unique<pqxx::connection>(DB_CONN_STR);

  if (!conn->is_open()) {
    throw std::runtime_error("Failed to connect to Postgres db");
  }

  return conn;
}

void getPapersFromDb(pqxx::work& tx, std::vector<ResearchPaper>& papers, const std::string topic, const uint32_t offset,
                     const uint32_t limit) {
  papers.clear();
  papers.reserve(limit);

  pqxx::result res =
      tx.exec_params(GET_PAPERS_QUERY, std::string(topic), static_cast<int>(limit), static_cast<int>(offset));

  for (const auto& row : res) {
    ResearchPaper paper;

    paper.id = row["id"].as<int64_t>();
    paper.source = stringToPaperSource(row["source"].as<std::string>());
    paper.source_id = row["source_id"].c_str();
    paper.title = row["title"].c_str();
    paper.pdf_url = row["pdf_url"].c_str();
    paper.doi = row["doi"].is_null() ? "" : row["doi"].c_str();
    paper.embedding_processed = row["embedding_processed"].as<bool>();
    paper.created_at = row["created_at"].c_str();
    paper.topic = row["topic"].c_str();

    // authors is JSONB array of strings → parse manually
    if (!row["authors"].is_null()) {
      std::string authors_json = row["authors"].c_str();
      authors_json.erase(std::remove(authors_json.begin(), authors_json.end(), '['), authors_json.end());
      authors_json.erase(std::remove(authors_json.begin(), authors_json.end(), ']'), authors_json.end());

      std::stringstream ss(authors_json);
      std::string author;
      while (std::getline(ss, author, ',')) {
        author.erase(remove(author.begin(), author.end(), '"'), author.end());
        paper.authors.push_back(author);
      }
    }

    papers.push_back(std::move(paper));
  }
}

void insert_embedding_chunk(pqxx::work& tx, const EmbeddingChunk& chunk) {
  tx.exec_params(INSERT_EMBEDDING_CHUNK_QUERY, chunk.faiss_id, chunk.document_id, chunk.chunk_index, chunk.page_number,
                 chunk.chunk_text, chunk.embedding_model);
  // int64_t db_id = row[0].as<int64_t>();
}

void insert_embedding_vector(pqxx::work& tx, const EmbeddingVector& ev) {
  const std::string embedding_str = to_pgvector(ev.embedding);

  tx.exec_params(INSERT_EMBEDDING_VECTOR_QUERY, ev.embedding_chunk_id, embedding_str);
}
