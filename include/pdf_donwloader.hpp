#pragma once

#include <curl/curl.h>
#include <curl/easy.h>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

void savePDFtoTextFile(std::string_view pdf_url, std::string_view file_path);
