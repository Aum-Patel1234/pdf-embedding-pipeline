#pragma once

#include <curl/curl.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexFlat.h>  // if you need the FAISS type here

#include <cmath>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../include/consts.hpp"
#include "../include/logger.hpp"
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
std::vector<std::vector<float>> getEmbeddings(const char* gemini_api_key_envname, std::string& MODEL_NAME,
                                              std::string& BATCH_URL, const std::vector<std::string_view>& chunks);

std::vector<std::vector<float>> get_vectors_from_response(const nlohmann::json& j, const size_t documents_size);

// std::vector<float> testEmbedding(const char* gemini_api_key_envname);
