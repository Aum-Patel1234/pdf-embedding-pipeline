#pragma once
#include <string>
#include <vector>

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
