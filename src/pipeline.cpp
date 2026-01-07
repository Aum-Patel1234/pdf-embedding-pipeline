#include "../include/pipeline.hpp"

#include <vector>

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
        std::cerr << "Failed to open output file.\n";
        return;
      }
      for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        ofs << "=== Chunk " << i << " (" << chunk.size() << " chars) ===\n";
        ofs.write(chunk.data(), chunk.size());
        ofs << "\n\n";
      }
      ofs.close();

      faiss::IndexFlatL2 index;
      std::vector<std::vector<float>> embeddings = getEmbeddings("GEMINI_API_KEY", index, chunks, researchPaper);
      printEmbeddings(embeddings);
    }

  } catch (const std::exception& e) {
    std::cerr << "PDF download failed: " << e.what() << '\n';
  }
}
