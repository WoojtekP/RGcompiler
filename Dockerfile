FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install system utilities, C++ build dependencies, Node.js, Python, and Binaryen (wasm-opt)
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    git \
    python3 \
    python3-pip \
    nlohmann-json3-dev \
    libboost-program-options-dev \
    libgtest-dev \
    binaryen \
    && rm -rf /var/lib/apt/lists/*

# 2. Install Node.js (v20 LTS)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs && \
    rm -rf /var/lib/apt/lists/*

# 3. Install CMake 3.30.5
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-linux-x86_64.tar.gz | \
    tar -xz --strip-components=1 -C /usr/local

# 4. Install Rust (Nightly toolchain) and add WASM target
# Nightly is required because the codebase relies on unstable Rust features like `extract_if`
ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    PATH=/usr/local/cargo/bin:$PATH

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain nightly && \
    rustup target add wasm32-unknown-unknown

# 5. Install wasm-pack
RUN curl https://rustwasm.github.io/wasm-pack/installer/init.sh -sSf | sh

WORKDIR /app

CMD ["bash"]