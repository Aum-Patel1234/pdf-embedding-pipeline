#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <string>

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

    setenv(key.c_str(), val.c_str(), 1);
  }
}
