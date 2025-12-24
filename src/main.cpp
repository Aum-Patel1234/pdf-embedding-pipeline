#include <faiss/IndexFlat.h>

#include <iostream>
#include <pqxx/pqxx>

#include "../include/embedding_engine/db.hpp"

int main() {
  // libpqxx: just declare a connection pointer (not connecting)
  pqxx::connection* conn = connectToDb();
  pqxx::work tx{*conn};

  int count = tx.query_value<int>("SELECT COUNT(*) FROM research_papers");
  std::cout << "Total research_papers = " << count << '\n';

  // FAISS: declare a simple index
  faiss::IndexFlatL2 index(768);  // 768-dim embeddings

  std::cout << "Build successful: libpqxx + FAISS linked\n";
  return 0;
}
