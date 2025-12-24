#include <iostream>

// libpqxx
#include <pqxx/pqxx>

// faiss
#include <faiss/IndexFlat.h>

int main() {
  // libpqxx: just declare a connection pointer (not connecting)
  pqxx::connection *conn = nullptr;

  // FAISS: declare a simple index
  faiss::IndexFlatL2 index(768); // 768-dim embeddings

  std::cout << "Build successful: libpqxx + FAISS linked\n";
  return 0;
}
