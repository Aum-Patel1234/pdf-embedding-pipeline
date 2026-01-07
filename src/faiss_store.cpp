#include <faiss/IndexFlat.h>

#include <iostream>
#include <vector>

void add_to_faiss(const std::vector<std::vector<float>>& all_embeddings) {
  if (all_embeddings.empty()) return;
  size_t dim = all_embeddings[0].size();

  // Use inner-product or L2 depending on similarity metric
  // For cosine: normalize vectors and use inner-product (approx cosine).
  faiss::IndexFlatIP index(dim);
}
