constexpr const char* GET_PAPERS_QUERY =
    "SELECT id, source, source_id, title, pdf_url, authors, doi, "
    "embedding_processed, created_at, topic "
    "FROM research_papers "
    "WHERE topic = $1 AND embedding_processed = false "
    "ORDER BY id "
    "LIMIT $2 OFFSET $3";

// constexpr const char* UPDATE_EMBEDDING_PROCESSED_QUERY =
//     "UPDATE research_papers "
//     "SET embedding_processed = true "
//     "WHERE id = $1";

constexpr const char* UPDATE_EMBEDDING_PROCESSED_QUERY =
    "UPDATE research_papers "
    "SET embedding_processed = true "
    "WHERE id = ANY($1::int[])";

// constexpr const char* INSERT_EMBEDDING_CHUNK_QUERY =
//     "INSERT INTO embedding_chunks ("
//     "    faiss_id, "
//     "    document_id, "
//     "    chunk_index, "
//     "    page_number, "
//     "    chunk_text, "
//     "    embedding_model"
//     ") "
//     "VALUES ($1, $2, $3, $4, $5, $6)"
//     "RETURNING id";

constexpr const char* INSERT_EMBEDDING_CHUNKS_QUERY =
    "INSERT INTO embedding_chunks ("
    "    document_id, "
    "    chunk_index, "
    "    page_number, "
    "    chunk_text, "
    "    embedding_model"
    ") "
    "SELECT "
    "    u.document_id, "
    "    u.chunk_index, "
    "    u.page_number, "
    "    u.chunk_text, "
    "    u.embedding_model "
    "FROM UNNEST ("
    "    $1::bigint[], "
    "    $2::int[], "
    "    $3::int[], "
    "    $4::text[], "
    "    $5::text[]"
    ") AS u("
    "    document_id, "
    "    chunk_index, "
    "    page_number, "
    "    chunk_text, "
    "    embedding_model"
    ") "
    "RETURNING id;";

// constexpr const char* INSERT_EMBEDDING_VECTOR_QUERY =
//     "INSERT INTO embedding_vectors ("
//     "    embedding_chunk_id, "
//     "    embedding"
//     ") "
//     "VALUES ($1, $2)";

constexpr const char* INSERT_EMBEDDING_VECTORS_QUERY =
    "INSERT INTO embedding_vectors ("
    "    embedding_chunk_id, "
    "    embedding"
    ") "
    "SELECT "
    "    t.embedding_chunk_id, "
    "    t.embedding::vector "
    "FROM UNNEST ("
    "    $1::bigint[], "
    "    $2::text[]"
    ") AS t(embedding_chunk_id, embedding);";

// TABLES

// -----------------------------------------------
// NOTE: SQL - embedding_chunks TABLE
// CREATE TABLE embedding_chunks (
//     id BIGSERIAL PRIMARY KEY,
//
//     -- FAISS vector ID (IndexIDMap ID) NOTE: deleted column
//     faiss_id BIGINT NOT NULL UNIQUE,
//
//     -- Reference to research_papers
//     document_id BIGINT NOT NULL,
//
//     -- Position of the chunk inside the document
//     chunk_index INT NOT NULL,
//
//     -- Optional but very useful
//     page_number INT,
//
//     -- The actual text used for embedding
//     chunk_text TEXT NOT NULL,
//
//     -- Embedding model identifier (versioning support)
//     embedding_model TEXT NOT NULL,
//
//     created_at TIMESTAMPTZ DEFAULT now(),
//
//     -- ---- Foreign key constraint ----
//     CONSTRAINT fk_embedding_chunks_document
//         FOREIGN KEY (document_id)
//         REFERENCES research_papers(id)
//         ON DELETE CASCADE
// );
//
// --Fast lookup when FAISS returns IDs CREATE INDEX idx_embedding_chunks_faiss_id ON embedding_chunks(faiss_id);
//
// --Group chunks per document CREATE INDEX idx_embedding_chunks_document_id ON embedding_chunks(document_id);
//
// --Ordered retrieval of chunks inside a document CREATE INDEX idx_embedding_chunks_doc_chunk ON
//     embedding_chunks(document_id, chunk_index);
// -- Drop the index on faiss_id (if it exists)
// DROP INDEX IF EXISTS idx_embedding_chunks_faiss_id;
//
// -- Drop the faiss_id column itself
// ALTER TABLE embedding_chunks
// DROP COLUMN IF EXISTS faiss_id;

// ----------------------table 2------------------
// CREATE TABLE embedding_vectors (
//     -- Primary key
//     id BIGSERIAL PRIMARY KEY,
//
//     -- Reference to embedding_chunks
//     embedding_chunk_id BIGINT NOT NULL,
//
//     -- The actual vector (768 dims)
//     embedding VECTOR(768) NOT NULL,
//
//     created_at TIMESTAMPTZ DEFAULT now(),
//
//     -- ---- Foreign key ----
//     CONSTRAINT fk_embedding_vectors_chunk
//         FOREIGN KEY (embedding_chunk_id)
//         REFERENCES embedding_chunks(id)
//         ON DELETE CASCADE
// );
// CREATE INDEX idx_embedding_vectors_embedding
// ON embedding_vectors
