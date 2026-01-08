cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(embedding_engine VERSION 0.1.0 LANGUAGES CXX)

if(UNIX)
    if(NOT DEFINED CMAKE_PREFIX_PATH OR CMAKE_PREFIX_PATH STREQUAL "")
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/vcpkg/installed/x64-linux/share" ${CMAKE_PREFIX_PATH})
    endif()
endif()

# On Windows, allow using VCPKG_ROOT environment variable for classic vcpkg.
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows/share")
    endif()
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Core deps
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if(WIN32)
    # Windows: vcpkg provides CONFIG files
    find_package(pqxx CONFIG REQUIRED)
    find_package(poppler-cpp CONFIG REQUIRED)

    set(PQXX_TARGET "")
    if(TARGET pqxx::pqxx)
        set(PQXX_TARGET pqxx::pqxx)
    elseif(TARGET libpqxx::libpqxx)
        set(PQXX_TARGET libpqxx::libpqxx)
    endif()

    if(PQXX_TARGET STREQUAL "")
        message(FATAL_ERROR
            "Could not find libpqxx (pqxx). Install libpqxx via vcpkg or add its share dir to CMAKE_PREFIX_PATH.\n"
            "  - For vcpkg (manifest): add \"libpqxx\" to vcpkg.json\n"
            "  - Or run: vcpkg install libpqxx:x64-windows\n"
            "  - Ensure CMake is invoked with -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
        )
    endif()

    # exposed variable PQXX_TARGET will be used when linking
    set(PQXX_TARGET ${PQXX_TARGET} CACHE STRING "pqxx target")
    message(STATUS "Using PQXX target: ${PQXX_TARGET}")
else()
    # Linux: prefer pkg-config, fallback to CONFIG
    find_package(PkgConfig QUIET)

    if(PkgConfig_FOUND)
        pkg_check_modules(PQXX REQUIRED libpqxx)
        pkg_check_modules(POPPLER REQUIRED poppler-cpp)

        if(NOT TARGET pqxx::pqxx)
            add_library(pqxx::pqxx INTERFACE IMPORTED)
            target_include_directories(pqxx::pqxx INTERFACE ${PQXX_INCLUDE_DIRS})
            target_link_libraries(pqxx::pqxx INTERFACE ${PQXX_LIBRARIES})
        endif()

        if(NOT TARGET poppler-cpp::poppler-cpp)
            add_library(poppler-cpp::poppler-cpp INTERFACE IMPORTED)
            target_include_directories(poppler-cpp::poppler-cpp INTERFACE ${POPPLER_INCLUDE_DIRS})
            target_link_libraries(poppler-cpp::poppler-cpp INTERFACE ${POPPLER_LIBRARIES})
        endif()
    else()
        find_package(pqxx CONFIG REQUIRED)
        find_package(poppler-cpp CONFIG REQUIRED)
    endif()
endif()


if(UNIX)
    # Try to locate FAISS
    find_path(FAISS_INCLUDE_DIR NAMES faiss/IndexFlat.h PATHS /usr/include /usr/local/include)
    find_library(FAISS_LIBRARY NAMES faiss faiss_avx2 faiss_avx512 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

    if(NOT FAISS_INCLUDE_DIR OR NOT FAISS_LIBRARY)
        message(FATAL_ERROR "FAISS not found. Install libfaiss-dev or build FAISS and set FAISS_INCLUDE_DIR/FAISS_LIBRARY.")
    endif()

    # Try to find BLAS/LAPACK via CMake modules
    find_package(BLAS QUIET)
    find_package(LAPACK QUIET)

    # If BLAS/LAPACK not found via find_package, try finding OpenBLAS directly
    set(EXTRA_LINK_LIBS "")
    if(DEFINED BLAS_LIBRARIES AND NOT "${BLAS_LIBRARIES}" STREQUAL "")
        list(APPEND EXTRA_LINK_LIBS ${BLAS_LIBRARIES})
    endif()
    if(DEFINED LAPACK_LIBRARIES AND NOT "${LAPACK_LIBRARIES}" STREQUAL "")
        list(APPEND EXTRA_LINK_LIBS ${LAPACK_LIBRARIES})
    endif()

    if(EXTRA_LINK_LIBS STREQUAL "")
        # try OpenBLAS explicit search
        find_library(OPENBLAS_LIB NAMES openblas libopenblas PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        if(OPENBLAS_LIB)
            list(APPEND EXTRA_LINK_LIBS ${OPENBLAS_LIB})
        endif()
    endif()

    # If still empty, warn and ask user to install libopenblas-dev or liblapack-dev.
    if(EXTRA_LINK_LIBS STREQUAL "")
        message(WARNING "No BLAS/LAPACK found automatically. FAISS may require OpenBLAS/LAPACK. Install libopenblas-dev or liblapack-dev.")
    endif()

    # Create imported target for FAISS if needed, and attach required link libraries
    if(NOT TARGET faiss::faiss)
        add_library(faiss::faiss UNKNOWN IMPORTED)
        set_target_properties(faiss::faiss PROPERTIES
            IMPORTED_LOCATION "${FAISS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FAISS_INCLUDE_DIR}"
            # Attach BLAS/LAPACK (if found) so linking pulls them in automatically
            INTERFACE_LINK_LIBRARIES "${EXTRA_LINK_LIBS}"
        )
    endif()

    # Create OpenBLAS imported target if found and no OpenBLAS::OpenBLAS target exists
    if(NOT TARGET OpenBLAS::OpenBLAS)
        if(OPENBLAS_LIB)
            add_library(OpenBLAS::OpenBLAS UNKNOWN IMPORTED)
            set_target_properties(OpenBLAS::OpenBLAS PROPERTIES
                IMPORTED_LOCATION "${OPENBLAS_LIB}"
                INTERFACE_INCLUDE_DIRECTORIES ""
            )
        endif()
    endif()
else()
    # Windows: prefer CONFIG-mode (vcpkg provides these targets)
    find_package(faiss CONFIG REQUIRED)
    find_package(OpenBLAS CONFIG REQUIRED)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(embedding_engine ${SOURCES})

target_include_directories(embedding_engine PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link libraries: ensure FAISS and BLAS/LAPACK appear in link set so unresolved symbols are resolved.
target_link_libraries(embedding_engine PRIVATE
    Threads::Threads
    OpenMP::OpenMP_CXX
    CURL::libcurl

    nlohmann_json::nlohmann_json
    pqxx::pqxx
    ${PQXX_TARGET}
    poppler-cpp::poppler-cpp
    faiss::faiss
)

# If BLAS/LAPACK variables exist, add them explicitly after the main link call to ensure proper resolution
if(UNIX)
    if(DEFINED BLAS_LIBRARIES AND NOT "${BLAS_LIBRARIES}" STREQUAL "")
        target_link_libraries(embedding_engine PRIVATE ${BLAS_LIBRARIES})
    endif()
    if(DEFINED LAPACK_LIBRARIES AND NOT "${LAPACK_LIBRARIES}" STREQUAL "")
        target_link_libraries(embedding_engine PRIVATE ${LAPACK_LIBRARIES})
    endif()
    if(OPENBLAS_LIB)
        target_link_libraries(embedding_engine PRIVATE ${OPENBLAS_LIB})
    endif()
endif()

if(MSVC)
    target_compile_options(embedding_engine PRIVATE /W4)
else()
    target_compile_options(embedding_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard    : ${CMAKE_CXX_STANDARD}")
message(STATUS "CMake prefix(s) : ${CMAKE_PREFIX_PATH}")
message(STATUS "Using vcpkg?    : ${CMAKE_TOOLCHAIN_FILE}")

if(UNIX)
    message(STATUS "On Linux: if CMake cannot find vcpkg-installed packages, either:")
    message(STATUS " - add $HOME/vcpkg/installed/x64-linux/share to CMAKE_PREFIX_PATH, or")
    message(STATUS " - pass -DCMAKE_TOOLCHAIN_FILE=\$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake to cmake")
endif()
if(WIN32)
    message(STATUS "On Windows: pass -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
