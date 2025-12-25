# Embedding Engine

C++ pipeline to fetch PDFs from database or web, split into chunks, and send to embedding server for ResearchQ.

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
