#include "../include/db.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
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

std::vector<int64_t> insert_embedding_chunks(pqxx::work& tx, const std::vector<EmbeddingChunk>& chunks) {
  std::vector<int64_t> document_ids;
  std::vector<int32_t> chunk_indexes;
  std::vector<uint32_t> page_numbers;
  std::vector<std::string> chunk_texts;
  std::vector<std::string> embedding_models;

  document_ids.reserve(chunks.size());
  chunk_indexes.reserve(chunks.size());
  page_numbers.reserve(chunks.size());
  chunk_texts.reserve(chunks.size());
  embedding_models.reserve(chunks.size());

  for (const auto& c : chunks) {
    document_ids.push_back(c.document_id);
    chunk_indexes.push_back(c.chunk_index);
    page_numbers.push_back(c.page_number);
    chunk_texts.push_back(c.chunk_text);
    embedding_models.push_back(c.embedding_model);
  }

  // batch insert
  auto result = tx.exec_params(INSERT_EMBEDDING_CHUNKS_QUERY, document_ids, chunk_indexes, page_numbers, chunk_texts,
                               embedding_models);

  std::vector<int64_t> inserted_ids;
  inserted_ids.reserve(result.size());

  for (const auto& row : result) inserted_ids.push_back(row[0].as<int64_t>());

  return inserted_ids;
}

void insert_embedding_vectors(pqxx::work& tx, const std::vector<EmbeddingVector>& vectors) {
  std::vector<int64_t> embedding_chunk_ids;
  std::vector<std::string> embeddings;

  embedding_chunk_ids.reserve(vectors.size());
  embeddings.reserve(vectors.size());

  for (const auto& ev : vectors) {
    embedding_chunk_ids.push_back(ev.embedding_chunk_id);
    embeddings.push_back(to_pgvector(ev.embedding));
  }

  tx.exec_params(INSERT_EMBEDDING_VECTORS_QUERY, embedding_chunk_ids, embeddings);
}
