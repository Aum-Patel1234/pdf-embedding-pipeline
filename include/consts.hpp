
// constexpr int TOTAL_PAPERS = 80649;  // i have in database
#include <cstdint>
constexpr uint8_t THREADS = 6;
// constexpr uint32_t LIMITS[6] = {13442, 13442, 13442, 13441, 13441, 13441};
constexpr uint32_t LIMITS[6] = {1, 0, 0, 0, 0, 0};
constexpr const char* GEMINI_API_KEY = "GEMINI_API_KEY";
constexpr const char* BATCH_URL =
    "https://generativelanguage.googleapis.com/v1beta/models/gemini-embedding-001:batchEmbedContents";
constexpr const char* MODEL_NAME = "models/gemini-embedding-001";
constexpr const char* TASK_TYPE = "RETRIEVAL_DOCUMENT";
constexpr int OUTPUT_DIM = 768;
constexpr std::size_t BATCH_SIZE = 16;
// constexpr int NLIST = 4096;
// constexpr int NPROBE = 16;
