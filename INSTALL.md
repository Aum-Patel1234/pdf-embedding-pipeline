### libpqxx-dev
    sudo apt-get install libpqxx-dev

### faiss

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  libopenblas-dev \
  liblapack-dev \
  libomp-dev \
  git


cd ~
git clone https://github.com/facebookresearch/faiss.git
cd faiss

mkdir build
cd build

cmake .. \
  -DFAISS_ENABLE_GPU=OFF \
  -DFAISS_ENABLE_PYTHON=OFF \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)


sudo make install
sudo ldconfig
# This installs:
# /usr/local/lib/libfaiss.so
# /usr/local/include/faiss/
# /usr/local/lib/pkgconfig/faiss.pc

# next see the CMakeLists.txt
```
