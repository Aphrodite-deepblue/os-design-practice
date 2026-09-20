FROM ubuntu:24.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential gcc-riscv64-linux-gnu qemu-system-misc \
    python3 perl bc git ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
