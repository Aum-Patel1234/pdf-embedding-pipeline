#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>

#include "consts.hpp"

#ifdef _WIN32
#include <cstdlib>
inline void set_env(const char* key, const char* value) { _putenv_s(key, value); }
#else
#include <cstdlib>
inline void set_env(const char* key, const char* value) { setenv(key, value, 1); }
#endif

inline void loadenv(const std::string& path = ".env") {
  std::ifstream file(path);
  std::string line;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    size_t pos = line.find('=');
    if (pos == std::string::npos) continue;

    // IMPORTANT: std::string_view does NOT own memory.
    // Using it with substr() creates a view to a temporary string that is destroyed immediately,
    // causing dangling references and undefined behavior. Use std::string to own the data.
    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);
    if (!val.empty() && val.front() == '"' && val.back() == '"') {
      val = val.substr(1, val.size() - 2);
    }

    set_env(key.c_str(), val.c_str());
  }
}

inline std::array<uint32_t, THREADS> getLimits(const uint8_t threads, const uint32_t totalPapers) {
  std::array<uint32_t, THREADS> limits;
  uint32_t mod = totalPapers % threads;
  uint32_t div = totalPapers / threads;

  for (uint32_t i = 0; i < THREADS; ++i) {
    limits[i] = (i < mod) ? div + 1 : div;
  }

  return limits;
}
