## Current

- [x] set up project (poppler, faiss, pgx)
- [x] implement download pdf from web and reading it
- [x] implement RecursiveTextSplitter
- [ ] read faiss docs  
  https://github.com/facebookresearch/faiss/wiki/Getting-started
- [ ] implement faiss vector store
  - vector size: 768
  - embedding model: Gemini text-embedding-004

## NOTE (https://ai.google.dev/gemini-api/docs/embeddings#rest)
 - RETRIEVAL_DOCUMENT 	Embeddings optimized for document search. 	Indexing articles, books, or web pages for search.
    - to store in faiss db

 - RETRIEVAL_QUERY 	Embeddings optimized for general search queries. Use RETRIEVAL_QUERY for queries; RETRIEVAL_DOCUMENT for documents to be retrieved. 
    - to get when in production


## Future - support for images

- while reading images the text splitter ignores it so supoort it and save it in S3.

## AIM URL to store to FAISS

```bash
curl "https://generativelanguage.googleapis.com/v1beta/models/gemini-embedding-001:batchEmbedContents" \
  -H "x-goog-api-key: $GEMINI_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "requests": [
      {
        "model": "models/gemini-embedding-001",
        "task_type": "RETRIEVAL_DOCUMENT",
        "content": {
          "parts": [
            { "text": "Chunk text from paper 1..." }
          ]
        },
        "output_dimensionality": 768
      },
      {
        "model": "models/gemini-embedding-001",
        "task_type": "RETRIEVAL_DOCUMENT",
        "content": {
          "parts": [
            { "text": "Another chunk text..." }
          ]
        },
        "output_dimensionality": 768
      }
    ]
  }'
```
