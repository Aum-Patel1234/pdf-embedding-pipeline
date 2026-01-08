#include "../include/pipeline.hpp"

#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>

#include <cassert>
#include <mutex>
#include <string>
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

void embedding_pipeline(const char* DB_CONN_STR, const std::string& topic, uint32_t offset, uint32_t limit) {
  std::unique_ptr<pqxx::connection> conn = connectToDb(DB_CONN_STR);
  pqxx::work tx{*conn};

  std::vector<ResearchPaper> papers;
  getPapersFromDb(tx, papers, topic, offset, limit);
  tx.commit();

  RecursiveCharacterTextSplitter r;
  faiss::IndexFlatIP* index = nullptr;

  try {
    for (auto& researchPaper : papers) {
      const std::string_view url = researchPaper.pdf_url;
      const std::string file_path = "temp/temp_file_" + std::to_string(offset) + ".pdf";

      savePDFtoTextFile(url, file_path);
      std::string content = read_file(file_path);
      content = r.cleanText(content);
      std::vector<std::string_view> chunks = r.getChunks(content);

      const std::string chunks_file = "temp/temp_chunks_" + std::to_string(offset) + ".txt";
      std::ofstream ofs(chunks_file, std::ios::out);

      if (!ofs) {
        logging::log_error("Failed to open output file.");
        return;
      }
      for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        ofs << "=== Chunk " << i << " (" << chunk.size() << " chars) ===\n";
        ofs.write(chunk.data(), chunk.size());
        ofs << "\n\n";
      }
      ofs.close();

      std::vector<float> embeddings = getEmbeddings(GEMINI_API_KEY, chunks, researchPaper);
      if (embeddings.empty()) continue;
      assert(embeddings.size() == OUTPUT_DIM * chunks.size());
      // printEmbeddings(embeddings);

      size_t n = embeddings.size() / OUTPUT_DIM;

      if (!index) {
        index = new faiss::IndexFlatIP(OUTPUT_DIM);
      }

      // ---- Normalize for cosine similarity ----
      faiss::fvec_renorm_L2(OUTPUT_DIM, n, embeddings.data());

      // ---- Add to FAISS ----
      index->add(n, embeddings.data());

      // ---- TODO: store metadata ----
      // storeChunkMetadata(researchPaper.id, chunks);

      std::cout << "Indexed paper: " << researchPaper.title << " | vectors: " << n << "\n";
    }

    if (index && index->ntotal > 0) {
      // TODO: make the faiss write threadsafe using mutex locks
      std::lock_guard<std::mutex> lock_guard(faiss_utils::faiss_index_mutex());
      faiss::write_index(index, "papers.faiss");
      logging::log_checkpoint("FAISS index saved. Total vectors: " + std::to_string(index->ntotal));
    }

    delete index;

  } catch (const std::exception& e) {
    logging::log_error("PDF download failed: " + std::string(e.what()));
  }
}
