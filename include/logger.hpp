#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>

namespace logging {
constexpr const char* ERROR_LOG_FILE = "app.log";
constexpr const char* CHECKPOINT_LOG_FILE = "checkpoint.log";

// IMPORTANT: the static var will store it in a static memmory and will be used same everytime
inline std::mutex& log_mutex() {
  static std::mutex mutex;
  return mutex;
}
inline std::mutex& checkpoint_mutex() {
  static std::mutex mutex;
  return mutex;
}

inline std::ofstream& error_log_stream() {
  static std::ofstream file(ERROR_LOG_FILE, std::ios::app);
  return file;
}

inline std::ofstream& checkpoint_log_stream() {
  static std::ofstream file(CHECKPOINT_LOG_FILE, std::ios::app);
  return file;
}

// threadsafe cause ctime is not
inline std::string timestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);

  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

// TODO: make this write threadsafe
inline void log_error(const std::string& msg) {
  std::lock_guard<std::mutex> lock_guard(log_mutex());
  error_log_stream() << "[ERROR] [" << timestamp() << "] " << msg << '\n';
}

inline void log_checkpoint(const std::string& msg) {
  std::lock_guard<std::mutex> lock_guard(checkpoint_mutex());
  checkpoint_log_stream() << "[CHECKPOINT] [" << timestamp() << "] " << msg << '\n';
}
}  // namespace logging
