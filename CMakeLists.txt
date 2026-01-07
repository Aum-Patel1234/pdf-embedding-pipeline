cmake_minimum_required(VERSION 3.16)

project(embedding_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_FAISS "Enable FAISS support (Linux/Windows via vcpkg)" OFF)

# Core dependencies
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(CURL REQUIRED)

if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED libpqxx)
    pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)
else()
    find_package(pqxx CONFIG REQUIRED)
    find_package(poppler-cpp CONFIG REQUIRED)
    find_package(faiss CONFIG OPTIONAL)
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(embedding_engine ${SOURCES})
target_include_directories(embedding_engine PRIVATE include)

if(UNIX)
    target_include_directories(embedding_engine PRIVATE ${PQXX_INCLUDE_DIRS} ${POPPLER_CPP_INCLUDE_DIRS})
endif()

set(ENGINE_LIBS Threads::Threads OpenMP::OpenMP_CXX CURL::libcurl)

if(UNIX)
    list(APPEND ENGINE_LIBS ${PQXX_LIBRARIES} ${POPPLER_CPP_LIBRARIES})
else()
    if(TARGET pqxx::pqxx)
        list(APPEND ENGINE_LIBS pqxx::pqxx)
    endif()
    if(TARGET poppler-cpp::poppler-cpp)
        list(APPEND ENGINE_LIBS poppler-cpp::poppler-cpp)
    endif()
endif()

# FAISS + OpenBLAS
if(USE_FAISS)
    if(UNIX)
        find_path(FAISS_INCLUDE_DIR NAMES faiss/IndexFlat.h PATHS /usr/local/include /usr/include)
        find_library(FAISS_LIBRARY NAMES faiss faiss_avx2 faiss_avx512 PATHS /usr/local/lib /usr/lib)

        if(NOT FAISS_INCLUDE_DIR OR NOT FAISS_LIBRARY)
            message(FATAL_ERROR "USE_FAISS=ON but FAISS not found. Install libfaiss-dev or build FAISS from source.")
        endif()

        find_package(OpenBLAS QUIET)
        if(NOT TARGET OpenBLAS::OpenBLAS)
            find_library(OPENBLAS_LIB NAMES openblas libopenblas PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
            find_path(OPENBLAS_INCLUDE_DIR NAMES cblas.h PATHS /usr/include /usr/local/include)
            if(OPENBLAS_LIB)
                add_library(OpenBLAS::OpenBLAS UNKNOWN IMPORTED)
                set_target_properties(OpenBLAS::OpenBLAS PROPERTIES IMPORTED_LOCATION "${OPENBLAS_LIB}")
                if(OPENBLAS_INCLUDE_DIR)
                    set_target_properties(OpenBLAS::OpenBLAS PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPENBLAS_INCLUDE_DIR}")
                endif()
            else()
                message(FATAL_ERROR "OpenBLAS not found. Install libopenblas-dev.")
            endif()
        endif()

        target_include_directories(embedding_engine PRIVATE ${FAISS_INCLUDE_DIR})
        list(APPEND ENGINE_LIBS ${FAISS_LIBRARY} OpenBLAS::OpenBLAS pthread)
    else()
        if(TARGET faiss::faiss)
            list(APPEND ENGINE_LIBS faiss::faiss)
        else()
            message(FATAL_ERROR "USE_FAISS=ON but FAISS not found on Windows.")
        endif()
    endif()
endif()

target_link_libraries(embedding_engine PRIVATE ${ENGINE_LIBS})

if(MSVC)
    target_compile_options(embedding_engine PRIVATE /W4 /WX)
else()
    target_compile_options(embedding_engine PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "USE_FAISS: ${USE_FAISS}")
if(UNIX)
    message(STATUS "PQXX libs: ${PQXX_LIBRARIES}")
    message(STATUS "POPPLER libs: ${POPPLER_CPP_LIBRARIES}")
endif()
