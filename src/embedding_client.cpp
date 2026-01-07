// embed_client.cpp
#include <curl/curl.h>

#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

// Helper for curl response capture
// static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
//   std::string* s = (std::string*)userp;
//   s->append((char*)contents, size * nmemb);
//   return size * nmemb;
// }
//
// int mainn() {
//   std::string api_key = std::getenv("GEMINI_API_KEY") ? std::getenv("GEMINI_API_KEY") : "";
//   if (api_key.empty()) {
//     std::cerr << "Set GEMINI_API_KEY env var\n";
//     return 1;
//   }
//
//   // Build JSON payload programmatically
//   json payload;
//   payload["requests"] = json::array();
//
//   // Example: push three chunks (in practice loop over PDF chunks)
//   payload["requests"].push_back(
//       {{"model", "models/gemini-embedding-001"}, {"content", {{"parts", {{{"text", "Chunk 1 text from PDF"}}}}}}});
//   payload["requests"].push_back(
//       {{"model", "models/gemini-embedding-001"}, {"content", {{"parts", {{{"text", "Chunk 2 text from PDF"}}}}}}});
//   payload["requests"].push_back(
//       {{"model", "models/gemini-embedding-001"}, {"content", {{"parts", {{{"text", "Chunk 3 text from PDF"}}}}}}});
//
//   std::string url =
//   "https://generativelanguage.googleapis.com/v1beta/models/gemini-embedding-001:batchEmbedContents";
//
//   CURL* curl = curl_easy_init();
//   if (!curl) {
//     std::cerr << "curl init failed\n";
//     return 1;
//   }
//
//   struct curl_slist* headers = nullptr;
//   std::string apiHeader = "x-goog-api-key: " + api_key;
//   headers = curl_slist_append(headers, apiHeader.c_str());
//   headers = curl_slist_append(headers, "Content-Type: application/json");
//
//   std::string resp;
//   curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.dump().c_str());
//   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
//   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
//
//   CURLcode res = curl_easy_perform(curl);
//   if (res != CURLE_OK) {
//     std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
//     curl_slist_free_all(headers);
//     curl_easy_cleanup(curl);
//     return 1;
//   }
//
//   long http_code = 0;
//   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
//   if (http_code < 200 || http_code >= 300) {
//     std::cerr << "HTTP error: " << http_code << "\nResponse body:\n" << resp << "\n";
//     curl_slist_free_all(headers);
//     curl_easy_cleanup(curl);
//     return 1;
//   }
//
//   // Parse JSON response
//   json j = json::parse(resp);
//   // The exact response shape may vary; docs show embeddings are available.
//   // Inspect j to learn exact key names; below is a robust approach:
//   std::vector<std::vector<float>> all_embeddings;
//   if (j.contains("responses")) {  // check for batch-style wrapper
//     for (auto& r : j["responses"]) {
//       if (r.contains("embedding") && r["embedding"].contains("values")) {
//         std::vector<float> emb;
//         for (auto& v : r["embedding"]["values"]) emb.push_back(v.get<float>());
//         all_embeddings.push_back(std::move(emb));
//       } else if (r.contains("embeddings")) {
//         // alternate key
//         for (auto& emb_j : r["embeddings"]) {
//           std::vector<float> emb;
//           for (auto& v : emb_j["values"]) emb.push_back(v.get<float>());
//           all_embeddings.push_back(std::move(emb));
//         }
//       }
//     }
//   } else if (j.contains("embeddings")) {  // direct embeddings array
//     for (auto& emb_j : j["embeddings"]) {
//       std::vector<float> emb;
//       for (auto& v : emb_j["values"]) emb.push_back(v.get<float>());
//       all_embeddings.push_back(std::move(emb));
//     }
//   } else {
//     // Fallback: try to find nested floats anywhere (inspect j)
//     std::cerr << "Unexpected response format, printing JSON:\n" << j.dump(2) << "\n";
//   }
//
//   std::cout << "Got " << all_embeddings.size() << " embeddings\n";
//
//   // cleanup
//   curl_slist_free_all(headers);
//   curl_easy_cleanup(curl);
//
//   // Example: print dims of first embedding
//   if (!all_embeddings.empty()) {
//     std::cout << "First embedding length: " << all_embeddings[0].size() << "\n";
//   }
//
//   // TODO: pass all_embeddings to FAISS (see next snippet)
//   return 0;
// }
