#pragma once
#include <string>

#if defined(__linux__) || defined(__APPLE__)

#include <uuid/uuid.h>

inline std::string generate_uuid() {
  uuid_t uuid;
  char uuid_str[37];

  uuid_generate(uuid);
  uuid_unparse_lower(uuid, uuid_str);

  return std::string(uuid_str);
}

#elif defined(_WIN32)

#include <rpc.h>
#include <windows.h>
#pragma comment(lib, "Rpcrt4.lib")

inline std::string generate_uuid() {
  UUID uuid;
  UuidCreate(&uuid);

  RPC_CSTR str = nullptr;
  UuidToStringA(&uuid, &str);

  std::string result(reinterpret_cast<char*>(str));
  RpcStringFreeA(&str);

  return result;
}

#else
#error "Unsupported platform for UUID generation"
#endif
