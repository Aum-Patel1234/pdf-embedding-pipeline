#include "../include/pipeline.hpp"

#include "consts.hpp"
#include "db.hpp"
#include "embeddings_client.hpp"
#include "logger.hpp"
#include "pdf_donwloader.hpp"
#include "pdf_processor.hpp"
#include "recursive_character_text_splitter.hpp"

// void printEmbeddings(const std::vector<std::vector<float>>& embeddings) {
//   for (size_t i = 0; i < embeddings.size(); ++i) {
//     std::cout << "Embedding " << i << " (" << embeddings[i].size() << " dims): [";
//     for (size_t j = 0; j < embeddings[i].size(); ++j) {
//       std::cout << embeddings[i][j];
//       if (j + 1 < embeddings[i].size()) std::cout << ", ";
//     }
//     std::cout << "]\n";
//   }
// }

void embedding_pipeline(const char* DB_CONN_STR, uint8_t thread_id, const std::string& topic, uint32_t offset,
                        uint32_t limit) {
  std::unique_ptr<pqxx::connection> conn = connectToDb(DB_CONN_STR);
  pqxx::work tx{*conn};

  std::vector<ResearchPaper> papers;
  getPapersFromDb(tx, papers, topic, offset, limit);

  RecursiveCharacterTextSplitter r;
  faiss::IndexFlatIP base_index(OUTPUT_DIM);
  faiss::IndexIDMap index(&base_index);
  const std::string file_path = "temp/temp_file_" + std::to_string(thread_id) + ".pdf";
  const std::string chunks_file = "temp/temp_chunks_" + std::to_string(thread_id) + ".txt";

  try {
    std::vector<int64_t> processed_ids;
    processed_ids.reserve(papers.size());

    for (auto& researchPaper : papers) {
      // step 1 - save the pdf to temp file
      savePDFtoTextFile(researchPaper.pdf_url, file_path);
      bool processed = false;

      // step 2 - read the pdf_file and extract text our of it
      std::vector<std::pair<uint32_t, std::string>> page_content = read_file(file_path);
      uint32_t chunk_index = 0;
      for (auto& [page_no, content] : page_content) {
        content = r.cleanText(content);

        // step 3 - get chunks out of raw content
        std::vector<std::string_view> meaningful_chunks = getMeaningfulChunks(r, content);
        if (meaningful_chunks.empty()) continue;

        // for debugging chunks
        write_to_temp_file_txt_to_debug(meaningful_chunks, chunks_file);
        // step 4 - get embeddings from the embedding_model
        auto result = get_embeddings_from_embedding_model(meaningful_chunks);
        std::vector<std::vector<float>>& vec_of_embeddings = result.first;
        if (vec_of_embeddings.size() != meaningful_chunks.size()) {
          logging::log_error("vector size and chunks size is different");
        }
        size_t n = vec_of_embeddings.size();
        std::vector<float>& flatten_embeddings = result.second;

        // step 5 -  TODO: store metadata and store the vector in embedding_vector table
        std::vector<faiss::idx_t> faiss_ids =
            storeChunksAndGetIds(tx, meaningful_chunks, researchPaper.id, page_no, chunk_index, MODEL_NAME);
        insert_embedding_vectors(tx, make_embedding_vectors(vec_of_embeddings, faiss_ids));

        // step 6 - add it to the faiss_index
        // ---- Normalize for cosine similarity ----
        faiss::fvec_renorm_L2(OUTPUT_DIM, n, flatten_embeddings.data());
        index.add_with_ids(n, flatten_embeddings.data(), faiss_ids.data());
        processed = true;
      }

      if (processed) processed_ids.push_back(researchPaper.id);
    }

    if (index.ntotal > 0) {
      // std::lock_guard<std::mutex> lock_guard(faiss_utils::faiss_index_mutex());
      // faiss::write_index(index.get(), "papers.faiss");
      const std::string index_path = "faiss/papers_" + std::to_string(thread_id) + ".faiss";
      faiss::write_index(&index, index_path.c_str());

      // for (int id : processed_ids) tx.exec_params(UPDATE_EMBEDDING_PROCESSED_QUERY, id);
      // Convert vector<int> to PostgreSQL array literal
      std::string pg_array = "{";
      for (size_t i = 0; i < processed_ids.size(); ++i) {
        if (i > 0) pg_array += ",";
        pg_array += std::to_string(processed_ids[i]);
      }
      pg_array += "}";

      // Execute mass update
      tx.exec_params(UPDATE_EMBEDDING_PROCESSED_QUERY, pg_array);
      // IMPORTANT: if the thing fails then nothing will be commited
      tx.commit();

      logging::log_checkpoint("FAISS index saved. Total vectors: " + std::to_string(index.ntotal));
    }

  } catch (const std::exception& e) {
    logging::log_error("PDF download failed: " + std::string(e.what()));
  }
}

std::pair<std::vector<std::vector<float>>, std::vector<float>> get_embeddings_from_embedding_model(
    const std::vector<std::string_view>& chunks) {
  // NOTE: do in batches as Gemini has rate-limit, see in consts.hpp
  std::vector<float> flatten_embeddings;
  std::vector<std::vector<float>> all_embeddings;
  all_embeddings.reserve(chunks.size());
  flatten_embeddings.reserve(chunks.size() * OUTPUT_DIM);

  for (size_t i = 0; i < chunks.size(); i += BATCH_SIZE) {
    size_t end = std::min(i + BATCH_SIZE, chunks.size());

    std::vector<std::string_view> batch(chunks.begin() + i, chunks.begin() + end);

    auto batch_embeddings = getEmbeddings(GEMINI_API_KEY, batch);
    if (batch_embeddings.empty()) {
      logging::log_error("Embedding batch failed");
      break;
    }

    for (const auto& emb : batch_embeddings) {
      if (emb.size() != OUTPUT_DIM) {
        logging::log_error("Invalid embedding dimension");
        continue;
      }

      all_embeddings.push_back(emb);
      flatten_embeddings.insert(flatten_embeddings.end(), emb.begin(), emb.end());
    }

    // rate-limit protection
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  if (all_embeddings.size() != chunks.size()) {
    logging::log_error("Embedding incomplete: got " + std::to_string(all_embeddings.size()) + ", expected " +
                       std::to_string(chunks.size()));
  }
  // printEmbeddings(embeddings);

  return {all_embeddings, flatten_embeddings};
};

void write_to_temp_file_txt_to_debug(const std::vector<std::string_view>& chunks, const std::string& chunks_file) {
  // go to start of the file
  std::ofstream ofs(chunks_file, std::ios::out | std::ios::app);
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
}
