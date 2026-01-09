#include <curl/curl.h>
#include <faiss/IndexFlat.h>  // if you need the FAISS type here

#include <cmath>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../include/consts.hpp"
#include "../include/embeddings_client.hpp"
#include "../include/logger.hpp"

using json = nlohmann::json;

// curl write callback
static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
  std::string* s = static_cast<std::string*>(userp);
  s->append(static_cast<char*>(contents), size * nmemb);
  return size * nmemb;
}

std::vector<float> getEmbeddings(const char* gemini_api_key_envname, const std::vector<std::string_view>& chunks,
                                 const ResearchPaper& paper) {
  const char* api_key_c = std::getenv(gemini_api_key_envname);
  if (!api_key_c) {
    logging::log_error("Environment variable '" + std::string(gemini_api_key_envname) + "' is not set.");
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
    logging::log_error("curl_easy_init() failed");
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
    logging::log_error("curl error: " + std::string(curl_easy_strerror(res)));
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code < 200 || http_code >= 300) {
    logging::log_error("HTTP error code: " + std::to_string(http_code) + "\nResponse body:\n" + resp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  // Parse response JSON
  json j;
  try {
    j = json::parse(resp);
  } catch (const std::exception& e) {
    logging::log_error("Failed to parse JSON response: " + std::string(e.what()) + "\nResponse body:\n" + resp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return {};
  }

  // std::vector<std::vector<float>> all_embeddings;
  // all_embeddings.reserve(documents.size());
  std::vector<float> all_embeddings;
  all_embeddings.reserve(OUTPUT_DIM * documents.size());

  // Response formats: top-level "responses" array (batch), or top-level "embeddings".
  if (j.contains("responses") && j["responses"].is_array()) {
    for (const auto& r : j["responses"]) {
      // primary format: r["embedding"]["values"]
      if (r.contains("embedding") && r["embedding"].contains("values")) {
        const auto& vals = r["embedding"]["values"];
        if (vals.size() != OUTPUT_DIM) {
          logging::log_error("Embedding dimension mismatch: got " + std::to_string(vals.size()) + ", expected " +
                             std::to_string(OUTPUT_DIM));
          return {};
        }
        for (const auto& v : vals) all_embeddings.push_back(v.get<float>());
      } else if (r.contains("embeddings") && r["embeddings"].is_array()) {
        // alternate: r["embeddings"] -> array of embedding objects
        for (const auto& embobj : r["embeddings"]) {
          if (embobj.contains("values")) {
            const auto& vals = embobj["values"];
            if (vals.size() != OUTPUT_DIM) {
              logging::log_error("Embedding dimension mismatch: got " + std::to_string(vals.size()) + ", expected " +
                                 std::to_string(OUTPUT_DIM));
              return {};
            }
            for (const auto& v : vals) all_embeddings.push_back(v.get<float>());
          }
        }
      } else {
        // unknown response node: print for debugging
        logging::log_error(std::string("Unexpected response entry (debug): ") + r.dump(2));
      }
    }
  } else if (j.contains("embeddings") && j["embeddings"].is_array()) {
    for (const auto& embobj : j["embeddings"]) {
      if (embobj.contains("values")) {
        const auto& vals = embobj["values"];
        if (vals.size() != OUTPUT_DIM) {
          logging::log_error("Embedding dimension mismatch: got " + std::to_string(vals.size()) + ", expected " +
                             std::to_string(OUTPUT_DIM));
          return {};
        }
        for (const auto& v : vals) all_embeddings.push_back(v.get<float>());
      }
    }
  } else {
    logging::log_error("Unexpected JSON response format:\n" + j.dump(2));
  }

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return all_embeddings;
}

// std::vector<float> testEmbedding(const char* gemini_api_key_envname) {
//   // Ensure env exists (sanity check)
//   if (!std::getenv(gemini_api_key_envname)) {
//     throw std::runtime_error("GEMINI API key not found in env");
//   }
//
//   // Stable storage for string_view
//   const std::string text = "This is a simple test sentence for embeddings.";
//   std::vector<std::string_view> chunks{std::string_view(text)};
//
//   ResearchPaper dummy{};
//   dummy.id = -1;
//   dummy.title = "embedding_test";
//   dummy.topic = "test";
//
//   std::vector<float> embeddings;
//
//   try {
//     embeddings = getEmbeddings(gemini_api_key_envname, chunks, dummy);
//   } catch (const std::exception& e) {
//     std::cerr << "[EMBED TEST ERROR] " << e.what() << "\n";
//     return {};
//   }
//
//   if (embeddings.empty()) {
//     std::cerr << "[EMBED TEST] Empty embedding returned\n";
//     return {};
//   }
//
//   if (embeddings.size() != OUTPUT_DIM) {
//     std::cerr << "[EMBED TEST] Unexpected dimension: " << embeddings.size() << " (expected " << OUTPUT_DIM << ")\n";
//     return {};
//   }
//
//   std::cout << "[EMBED TEST] SUCCESS\n";
//   std::cout << "Dimension: " << embeddings.size() << "\n";
//   std::cout << "First 5 values: ";
//   for (size_t i = 0; i < 5; ++i) {
//     std::cout << embeddings[i] << " ";
//   }
//   std::cout << "\n";
//
//   return embeddings;
// }
