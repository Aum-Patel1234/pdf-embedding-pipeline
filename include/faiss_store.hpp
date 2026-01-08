#include <mutex>

namespace faiss_utils {

inline std::mutex& faiss_index_mutex() {
  static std::mutex m;
  return m;
}

}  // namespace faiss_utils
