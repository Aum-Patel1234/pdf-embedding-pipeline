#include "../include/pipeline.hpp"

#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>

#include <cassert>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../include/consts.hpp"
#include "../include/embeddings_client.hpp"
#include "../include/faiss_store.hpp"
#include "../include/logger.hpp"
#include "../include/pdf_donwloader.hpp"
#include "../include/pdf_processor.hpp"
#include "../include/recursive_character_text_splitter.hpp"

void printEmbeddings(const std::vector<std::vector<float>>& embeddings) {
  for (size_t i = 0; i < embeddings.size(); ++i) {
    std::cout << "Embedding " << i << " (" << embeddings[i].size() << " dims): [";
    for (size_t j = 0; j < embeddings[i].size(); ++j) {
      std::cout << embeddings[i][j];
      if (j + 1 < embeddings[i].size()) std::cout << ", ";
    }
    std::cout << "]\n";
  }
}

void embedding_pipeline(const char* DB_CONN_STR, uint8_t thread_id, const std::string& topic, uint32_t offset,
                        uint32_t limit) {
  std::unique_ptr<pqxx::connection> conn = connectToDb(DB_CONN_STR);
  pqxx::work tx{*conn};

  std::vector<ResearchPaper> papers;
  getPapersFromDb(tx, papers, topic, offset, limit);

  RecursiveCharacterTextSplitter r;
  faiss::IndexFlatIP index(OUTPUT_DIM);
  const std::string file_path = "temp/temp_file_" + std::to_string(thread_id) + ".pdf";
  const std::string chunks_file = "temp/temp_chunks_" + std::to_string(thread_id) + ".txt";

  try {
    std::vector<int> processed_ids;
    processed_ids.reserve(papers.size());

    for (auto& researchPaper : papers) {
      savePDFtoTextFile(researchPaper.pdf_url, file_path);

      std::string content = read_file(file_path);
      content = r.cleanText(content);

      // NOTE: can lead to lifetime_issues but currently good
      std::vector<std::string_view> chunks = r.getChunks(content);

      // go to start of the file
      std::ofstream ofs(chunks_file, std::ios::out | std::ios::trunc);
      if (!ofs) {
        logging::log_error("Failed to open output file.");
        continue;
      }

      for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        ofs << "=== Chunk " << i << " (" << chunk.size() << " chars) ===\n";
        ofs.write(chunk.data(), chunk.size());
        ofs << "\n\n";
      }

      // NOTE: do in batches as Gemini has rate-limit
      std::vector<float> embeddings;
      embeddings.reserve(chunks.size() * OUTPUT_DIM);
      for (size_t i = 0; i < chunks.size(); i += BATCH_SIZE) {
        size_t end = std::min(i + BATCH_SIZE, chunks.size());

        std::vector<std::string_view> batch(chunks.begin() + i, chunks.begin() + end);

        auto emb = getEmbeddings(GEMINI_API_KEY, batch, researchPaper);
        if (emb.empty()) {
          logging::log_error("Embedding batch failed");
          break;
        }

        embeddings.insert(embeddings.end(), emb.begin(), emb.end());

        // rate-limit protection
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }

      if (embeddings.size() != OUTPUT_DIM * chunks.size()) {
        logging::log_error("Embedding incomplete: got " + std::to_string(embeddings.size()) + ", expected " +
                           std::to_string(OUTPUT_DIM * chunks.size()));
        return;  // or continue to next paper
      }
      // printEmbeddings(embeddings);

      size_t n = embeddings.size() / OUTPUT_DIM;

      // ---- Normalize for cosine similarity ----
      faiss::fvec_renorm_L2(OUTPUT_DIM, n, embeddings.data());

      // ---- Add to FAISS ----
      index.add(n, embeddings.data());
      processed_ids.push_back(researchPaper.id);

      // ---- TODO: store metadata ----
      // storeChunkMetadata(researchPaper.id, chunks);

      // std::cout << "Indexed paper: " << researchPaper.title << " | vectors: " << n << "\n";
    }

    if (index.ntotal > 0) {
      // std::lock_guard<std::mutex> lock_guard(faiss_utils::faiss_index_mutex());
      // faiss::write_index(index.get(), "papers.faiss");
      const std::string index_path = "faiss/papers_" + std::to_string(thread_id) + ".faiss";
      faiss::write_index(&index, index_path.c_str());

      for (int id : processed_ids) tx.exec_params(UPDATE_EMBEDDING_PROCESSED_QUERY, id);
      tx.commit();

      logging::log_checkpoint("FAISS index saved. Total vectors: " + std::to_string(index.ntotal));
    }

  } catch (const std::exception& e) {
    logging::log_error("PDF download failed: " + std::string(e.what()));
  }
}
