#include "../include/pipeline.hpp"

#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>

#include <vector>

#include "../include/consts.hpp"
#include "../include/logger.hpp"
#include "../include/pdf_donwloader.hpp"
#include "../include/pdf_processor.hpp"
#include "../include/recursive_character_text_splitter.hpp"
#include "embeddings_client.hpp"

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
  size_t embedding_dim = 0;

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
        log_error("Failed to open output file.");
        return;
      }
      for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        ofs << "=== Chunk " << i << " (" << chunk.size() << " chars) ===\n";
        ofs.write(chunk.data(), chunk.size());
        ofs << "\n\n";
      }
      ofs.close();

      std::vector<std::vector<float>> embeddings = getEmbeddings(GEMINI_API_KEY, chunks, researchPaper);
      if (embeddings.empty()) continue;
      // printEmbeddings(embeddings);

      if (!index) {
        embedding_dim = embeddings[0].size();
        index = new faiss::IndexFlatIP(embedding_dim);
      }

      size_t n = embeddings.size();
      size_t dim = embedding_dim;

      // ---- Flatten embeddings ----
      std::vector<float> flat;
      flat.reserve(n * dim);
      for (const auto& e : embeddings) flat.insert(flat.end(), e.begin(), e.end());

      // ---- Normalize for cosine similarity ----
      faiss::fvec_renorm_L2(dim, n, flat.data());

      // ---- Add to FAISS ----
      index->add(n, flat.data());

      // ---- TODO: store metadata ----
      // storeChunkMetadata(researchPaper.id, chunks);

      std::cout << "Indexed paper: " << researchPaper.title << " | vectors: " << n << "\n";
    }

    if (index && index->ntotal > 0) {
      faiss::write_index(index, "papers.faiss");
      std::cout << "FAISS index saved. Total vectors: " << index->ntotal << "\n";
    }

    delete index;

  } catch (const std::exception& e) {
    log_error("PDF download failed: " + std::string(e.what()));
  }
}
