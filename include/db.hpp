#pragma once

#include <pqxx/pqxx>
#include <stdexcept>

constexpr const char* DB_CONN_STR =
    "host=localhost "
    "port=5432 "
    "dbname=final_year_rag "
    "user=postgres "
    "password=postgres";

pqxx::connection* connectToDb();
