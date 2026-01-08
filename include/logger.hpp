#include <fstream>
inline std::ofstream& log_stream() {
  static std::ofstream file("app.log", std::ios::app);  // app ->seek to end before each write
  return file;
}

inline void log_error(const std::string& msg) {
  time_t t = time(nullptr);
  log_stream() << "[ERROR] " << ctime(&t) << " " << msg << "\n";
}
