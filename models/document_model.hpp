#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

enum class PaperSource { ARXIV, SEMANTIC_SCHOLAR, SPRINGER_NATURE };

inline PaperSource stringToPaperSource(const std::string& s) {
  if (s == "arxiv") return PaperSource::ARXIV;
  if (s == "semantic_scholar") return PaperSource::SEMANTIC_SCHOLAR;
  if (s == "springer_nature") return PaperSource::SPRINGER_NATURE;
  throw std::runtime_error("Unknown paper source: " + s);
}

struct ResearchPaper {
  int64_t id;
  PaperSource source;
  std::string source_id;
  std::string title;
  std::string pdf_url;
  std::vector<std::string> authors;
  std::string doi;
  bool embedding_processed = false;
  std::string created_at;
  std::string topic;
};

struct Document {
  int64_t id;           // can be uint64_t but faiss expects int64_t so didnt changed
  std::string content;  // full text of paper or chunk

  PaperSource source;
  std::string title;
  std::string pdf_url;
  std::string topic;
  std::vector<std::string> authors;

  // bool embedding_processed = false;  // has embedding been generated
};
