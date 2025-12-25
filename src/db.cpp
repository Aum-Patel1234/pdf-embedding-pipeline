#include "../include/db.hpp"

pqxx::connection* connectToDb() {
  static pqxx::connection conn(DB_CONN_STR);

  if (!conn.is_open()) throw std::runtime_error("Failed to connect to Postgres db");

  return &conn;
}
