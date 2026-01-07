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

# -------------------- Options --------------------
option(USE_FAISS "Enable FAISS support (Linux only)" OFF)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(CURL REQUIRED)
# find_package(PkgConfig REQUIRED)

# -------------------- Platform-specific deps --------------------
if(UNIX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(PQXX REQUIRED libpqxx)
    pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)
else()
    # Windows (via vcpkg)
    find_package(pqxx CONFIG REQUIRED)
    find_package(poppler-cpp CONFIG REQUIRED)
endif()

# # PostgreSQL (libpqxx)
# pkg_check_modules(PQXX REQUIRED libpqxx)
# pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(embedding_engine ${SOURCES})

target_include_directories(embedding_engine
    PRIVATE
        include
        ${PQXX_INCLUDE_DIRS}
        ${POPPLER_CPP_INCLUDE_DIRS}
        # /usr/local/include        # FAISS headers
)

# -------------------- FAISS (Linux only, optional) --------------------
if(USE_FAISS AND UNIX)
    target_include_directories(embedding_engine PRIVATE /usr/local/include)
    target_link_libraries(embedding_engine PRIVATE /usr/local/lib/libfaiss.a openblas)
endif()

target_link_libraries(embedding_engine
    PRIVATE
        Threads::Threads
        OpenMP::OpenMP_CXX
        ${PQXX_LIBRARIES}
        # /usr/local/lib/libfaiss.a   # link it manually see README.md 
        # openblas
        CURL::libcurl
        ${POPPLER_CPP_LIBRARIES} 
)

if(MSVC)
    target_compile_options(embedding_engine PRIVATE /W4)
else()
    target_compile_options(embedding_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
