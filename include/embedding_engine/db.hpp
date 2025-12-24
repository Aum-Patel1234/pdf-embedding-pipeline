#pragma once

#include <pqxx/pqxx>
#include <stdexcept>

pqxx::connection* connectToDb();
