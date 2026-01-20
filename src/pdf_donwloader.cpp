#include "../include/pdf_donwloader.hpp"

namespace fs = std::filesystem;

#include <thread>

fs::path makeThreadTempFile(const fs::path& final_path) {
  std::stringstream ss;
  ss << final_path.string() << ".thread" << std::this_thread::get_id() << ".tmp";
  return ss.str();
}

// IMPORTANT: https://everything.curl.dev/transfers/callbacks/write.html
// contents → pointer to the raw bytes received (as void*)
// size → size of each element (usually 1)
// nmemb → number of elements (so total bytes = size * nmemb)
// userp → custom pointer (e.g., std::ofstream*)
static size_t write_callback(void* contents, size_t size, size_t nmeb, void* userp) {
  std::ofstream* ofs = static_cast<std::ofstream*>(userp);
  ofs->write(static_cast<char*>(contents), size * nmeb);
  return size * nmeb;
}

// NOTE: wrap this function with try catch
// curl_easy_init() → create a request object
// CURLOPT_URL → define what to download
// CURLOPT_WRITEFUNCTION → define how data is consumed
// CURLOPT_WRITEDATA → define where data goes
// CURLOPT_FOLLOWLOCATION → handle real-world HTTP behavior
// curl_easy_perform()  → execute the whole pipeline synchronously
void savePDFtoTextFile(std::string_view pdf_url, std::string_view file_path) {
  fs::path path{file_path};

  // ensure parent directory exists
  if (path.has_parent_path()) {
    fs::create_directories(path.parent_path());
  }

  // temp file unique per thread
  fs::path tmp_path = makeThreadTempFile(path);

  std::ofstream ofs(tmp_path, std::ios::binary);
  if (!ofs) throw std::runtime_error("Failed to open output file");

  CURL* curl = curl_easy_init();
  if (!curl) throw std::runtime_error("Failed to init curl");

  curl_easy_setopt(curl, CURLOPT_URL, pdf_url.data());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  ofs.close();

  if (res != CURLE_OK) {
    fs::remove(tmp_path);  // remove partial file
    throw std::runtime_error(curl_easy_strerror(res));
  }

  // check PDF header
  std::ifstream ifs(tmp_path, std::ios::binary);
  char hdr[4] = {};
  ifs.read(hdr, 4);
  ifs.close();
  if (std::string(hdr, 4) != "%PDF") {
    fs::remove(tmp_path);
    throw std::runtime_error("Downloaded file is not a valid PDF: " + std::string(pdf_url));
  }

  // Move to final path (overwrite if exists)
  fs::rename(tmp_path, path);
}
