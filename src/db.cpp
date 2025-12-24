#include "../include/embedding_engine/db.hpp"

pqxx::connection* connectToDb() {
  static pqxx::connection conn(
      "host=localhost "
      "port=5432 "
      "dbname=final_year_rag "
      "user=postgres "
      "password=postgres");

  if (!conn.is_open()) throw std::runtime_error("Failed to connect to Postgres db");

  return &conn;
}
