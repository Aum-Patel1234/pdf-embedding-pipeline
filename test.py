import os
import requests
import faiss
import numpy as np

# ==============================
# CONFIG
# ==============================
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")
if not GEMINI_API_KEY:
    raise RuntimeError("GEMINI_API_KEY not set")

FAISS_INDEX_PATH = "paper.faiss"
EMBEDDING_DIM = 768
TOP_K = 5

EMBEDDING_URL = (
    "https://generativelanguage.googleapis.com/v1beta/"
    "models/gemini-embedding-001:batchEmbedContents"
)


CHAT_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent"


HEADERS = {
    "Content-Type": "application/json",
    "x-goog-api-key": GEMINI_API_KEY,
}


# ==============================
# STEP 1: QUERY EMBEDDING
# ==============================
def embed_query(query_text: str) -> np.ndarray:
    payload = {
        "requests": [
            {
                "model": "models/gemini-embedding-001",
                "task_type": "RETRIEVAL_QUERY",
                "content": {"parts": [{"text": query_text}]},
                "output_dimensionality": EMBEDDING_DIM,
            }
        ]
    }

    resp = requests.post(EMBEDDING_URL, headers=HEADERS, json=payload)
    resp.raise_for_status()

    data = resp.json()
    vec = np.array(data["embeddings"][0]["values"], dtype="float32")

    if vec.shape[0] != EMBEDDING_DIM:
        raise RuntimeError("Embedding dimension mismatch")

    # Normalize for cosine similarity
    vec = vec.reshape(1, -1)
    faiss.normalize_L2(vec)
    return vec


# ==============================
# STEP 2: LOAD FAISS
# ==============================
def load_faiss_index(path: str) -> faiss.Index:
    index = faiss.read_index(path)
    if index.d != EMBEDDING_DIM:
        raise RuntimeError("FAISS dimension mismatch")
    return index


# ==============================
# STEP 3: FAISS SEARCH
# ==============================
def search_faiss(index: faiss.Index, query_vec: np.ndarray, k: int):
    scores, ids = index.search(query_vec, k)
    return scores[0], ids[0]


# ==============================
# STEP 4: FETCH CHUNKS
# ==============================
def fetch_chunks_by_ids(ids):
    """
    Replace this with:
    - DB lookup
    - file lookup
    - metadata store

    For now, mock data.
    """
    fake_db = {
        0: "Transformers are neural network architectures based on self-attention.",
        1: "Transformer models are widely used for machine translation and summarization.",
        2: "The transformer architecture eliminates recurrence and convolution.",
        3: "Attention mechanisms allow transformers to model long-range dependencies.",
        4: "Transformers scale efficiently on large datasets.",
    }

    chunks = []
    for idx in ids:
        if idx in fake_db:
            chunks.append(fake_db[idx])
    return chunks


# ==============================
# STEP 5: GEMINI CHAT
# ==============================
def ask_gemini(context_chunks, question):
    context_text = "\n\n".join(context_chunks)

    prompt = f"""
Use the following context to answer the question.

Context:
{context_text}

Question:
{question}
"""

    payload = {"contents": [{"role": "user", "parts": [{"text": prompt}]}]}

    resp = requests.post(CHAT_URL, headers=HEADERS, json=payload)
    resp.raise_for_status()

    data = resp.json()
    return data["candidates"][0]["content"]["parts"][0]["text"]


# ==============================
# MAIN PIPELINE
# ==============================
def main():
    query = "What are transformer models used for in natural language processing?"

    print("\n[1] Query:")
    print(query)

    print("\n[2] Getting query embedding from Gemini...")
    query_vec = embed_query(query)

    print("\n[3] Loading FAISS index...")
    index = load_faiss_index(FAISS_INDEX_PATH)
    print(f"Total vectors in index: {index.ntotal}")

    print("\n[4] Searching FAISS...")
    scores, ids = search_faiss(index, query_vec, TOP_K)

    print("\nTop matches:")
    for i in range(len(ids)):
        print(f"Rank {i+1} | FAISS ID {ids[i]} | Score {scores[i]}")

    print("\n[5] Fetching chunk texts...")
    chunks = fetch_chunks_by_ids(ids)

    print("\nRetrieved context:")
    for i, c in enumerate(chunks):
        print(f"\n--- Chunk {i+1} ---\n{c}")

    print("\n[6] Asking Gemini for final answer...")
    answer = ask_gemini(chunks, query)

    print("\n================ FINAL ANSWER ================\n")
    print(answer)
    print("\n=============================================\n")


if __name__ == "__main__":
    main()
