#include <curl/curl.h>
#include <faiss/IndexFlat.h>  // if you need the FAISS type here

#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../include/embeddings_client.hpp"

constexpr const char* BATCH_URL =
    "https://generativelanguage.googleapis.com/v1beta/models/gemini-embedding-001:batchEmbedContents";
constexpr const char* MODEL_NAME = "models/gemini-embedding-001";
constexpr const char* TASK_TYPE = "RETRIEVAL_DOCUMENT";
constexpr int OUTPUT_DIM = 768;

using json = nlohmann::json;

// curl write callback
static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
  std::string* s = static_cast<std::string*>(userp);
  s->append(static_cast<char*>(contents), size * nmemb);
  return size * nmemb;
}

std::vector<std::vector<float>> getEmbeddings(const char* gemini_api_key_envname, faiss::IndexFlatL2& index,
                                              const std::vector<std::string_view>& chunks, const ResearchPaper& paper) {
  const char* api_key_c = std::getenv(gemini_api_key_envname);
  if (!api_key_c) {
    std::cerr << "Environment variable '" << gemini_api_key_envname << "' is not set.\n";
    return {};
  }
  std::string api_key(api_key_c);

  // Build Documents from chunks
  std::vector<Document> documents = getDocumentsFromChunks(chunks, paper);

  // Build JSON payload
  json payload;
  payload["requests"] = json::array();

  for (const auto& doc : documents) {
    json req;
    req["model"] = MODEL_NAME;
    req["task_type"] = TASK_TYPE;
    req["output_dimensionality"] = OUTPUT_DIM;

    req["content"] = json::object();
    req["content"]["parts"] = json::array();

    // Each request corresponds to one chunk/document; keep single-part for clear chunk->embedding mapping
    json part;
    part["text"] = doc.content;
    req["content"]["parts"].push_back(part);

    payload["requests"].push_back(std::move(req));
  }

  // Initialize CURL
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "curl_easy_init() failed\n";
    return {};
  }

  struct curl_slist* headers = nullptr;
  std::string apiheader = std::string("x-goog-api-key: ") + api_key;
  headers = curl_slist_append(headers, apiheader.c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::string resp;
  std::string payload_str = payload.dump();

  curl_easy_setopt(curl, CURLOPT_URL, BATCH_URL);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload_str.size()));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code < 200 || http_code >= 300) {
    std::cerr << "HTTP error code: " << http_code << "\nResponse body:\n" << resp << "\n";
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  // Parse response JSON
  json j;
  try {
    j = json::parse(resp);
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse JSON response: " << e.what() << "\nResponse body:\n" << resp << "\n";
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  std::vector<std::vector<float>> all_embeddings;
  all_embeddings.reserve(documents.size());

  // Response formats: top-level "responses" array (batch), or top-level "embeddings".
  if (j.contains("responses") && j["responses"].is_array()) {
    for (const auto& r : j["responses"]) {
      // primary format: r["embedding"]["values"]
      if (r.contains("embedding") && r["embedding"].contains("values")) {
        const auto& vals = r["embedding"]["values"];
        std::vector<float> emb;
        emb.reserve(vals.size());
        for (const auto& v : vals) emb.push_back(v.get<float>());
        all_embeddings.push_back(std::move(emb));
      } else if (r.contains("embeddings") && r["embeddings"].is_array()) {
        // alternate: r["embeddings"] -> array of embedding objects
        for (const auto& embobj : r["embeddings"]) {
          if (embobj.contains("values")) {
            const auto& vals = embobj["values"];
            std::vector<float> emb;
            emb.reserve(vals.size());
            for (const auto& v : vals) emb.push_back(v.get<float>());
            all_embeddings.push_back(std::move(emb));
          }
        }
      } else {
        // unknown response node: print for debugging
        std::cerr << "Unexpected response entry (debug): " << r.dump(2) << "\n";
      }
    }
  } else if (j.contains("embeddings") && j["embeddings"].is_array()) {
    for (const auto& embobj : j["embeddings"]) {
      if (embobj.contains("values")) {
        const auto& vals = embobj["values"];
        std::vector<float> emb;
        emb.reserve(vals.size());
        for (const auto& v : vals) emb.push_back(v.get<float>());
        all_embeddings.push_back(std::move(emb));
      }
    }
  } else {
    std::cerr << "Unexpected JSON response format:\n" << j.dump(2) << "\n";
  }

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  // Optionally validate dimensionality
  for (size_t i = 0; i < all_embeddings.size(); ++i) {
    if (static_cast<int>(all_embeddings[i].size()) != OUTPUT_DIM) {
      std::cerr << "Warning: embedding " << i << " has size " << all_embeddings[i].size() << " (expected " << OUTPUT_DIM
                << ")\n";
    }
  }

  return all_embeddings;
}
