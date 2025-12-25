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

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# PostgreSQL (libpqxx)
pkg_check_modules(PQXX REQUIRED libpqxx)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(embedding_engine ${SOURCES})

target_include_directories(embedding_engine
    PRIVATE
        include
        ${PQXX_INCLUDE_DIRS}
        /usr/local/include        # FAISS headers
)

target_link_libraries(embedding_engine
    PRIVATE
        Threads::Threads
        OpenMP::OpenMP_CXX
        ${PQXX_LIBRARIES}
        /usr/local/lib/libfaiss.a   # link it manually see README.md 
        openblas
        CURL::libcurl
)

target_compile_options(embedding_engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
