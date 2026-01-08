#pragma once
#include <faiss/IndexFlat.h>

#include <string>
#include <vector>

#include "../models/document_model.hpp"

struct ContentPart {
  std::string text;
};

struct Content {
  std::vector<ContentPart> parts;
};

struct EmbedRequestItem {
  std::string model;  // e.g. "models/gemini-embedding-004"
  Content content;
};

struct BatchEmbedRequest {
  std::vector<EmbedRequestItem> requests;
};

// Response: each embedding entry commonly contains a vector of floats/doubles.
// We'll model it as vector<float>.
struct Embedding {
  std::vector<float> values;
};

struct BatchEmbedResponse {
  std::vector<Embedding> embeddings;  // one per request
};

// URL: https://ai.google.dev/gemini-api/docs/embeddings#rest
std::vector<float> getEmbeddings(const char* gemini_api_key_envname, const std::vector<std::string_view>& chunks,
                                 const ResearchPaper& paper);
