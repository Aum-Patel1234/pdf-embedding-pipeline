# Embedding Engine

A high-performance **C++ embedding pipeline** for research paper ingestion and retrieval.  
The engine reads paper metadata from a database, processes PDFs into chunks, generates embeddings using an external embedding service, and stores them in **FAISS** for efficient similarity search.

This project is a core component of the **Final Year RAG (Retrieval-Augmented Generation)** system.

Related project: https://github.com/Aum-Patel1234/researchq

---

## Features

- C++-based embedding ingestion pipeline
- PostgreSQL metadata access using `libpqxx`
- PDF chunking and preprocessing
- External embedding service integration
- Vector indexing and storage with FAISS
- Persistent local vector store

---

## Tech Stack

- C++17
- FAISS
- libpqxx
- CMake
- PostgreSQL

---

## Setup

See `INSTALL.md` for build and setup instructions.