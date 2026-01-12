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

## Setup (to rebuild using cmake)

See `INSTALL.md` for build and setup instructions.

## Installation (via Release ZIP ONLY for Windows)

You can install the Embedding Engine without building from source by using the prebuilt binaries available in **GitHub Releases**.

### 1. Download Release
- Go to the **Releases** page  
- Download the latest `embedding_engine.zip`
- Extract it:

```bash
unzip embedding_engine.zip
cd embedding_engine
```

### 2. make .env

```bash
DATABASE_URL=
GEMINI_API_KEY=
TOPIC="natural language processing"
BATCH_URL=
MODEL_NAME=
# DATABASE_URL=postgresql://postgres:postgres@localhost:5432/final_year_rag
```
