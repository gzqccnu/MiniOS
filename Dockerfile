# Lrix
# Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
# 
# This program is released under the terms of the GNU General Public License version 2(GPLv2).
# See https://opensource.org/licenses/GPL-2.0 for more information.
# 
# Project homepage: https://github.com/lrisguan/Lrix
# Description: A scratch implemention of OS based on RISC-V

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Basic dependencies: certificates, download tools, archive tools, build tools, 
# Python3, QEMU RISC-V, and runtime libs for the RISC-V toolchain
RUN apt-get update && apt-get install -y \
    ca-certificates \
    curl \
    xz-utils \
    make \
    python3 \
    qemu-system-riscv64 \
    libmpc3 \
    libmpfr6 \
    libgmp10 \
 && rm -rf /var/lib/apt/lists/*

# Install the RISC-V cross toolchain
# extract the prebuilt archive
RUN mkdir -p /opt \
 && cd /opt \
 && curl -L https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2026.01.09/riscv64-elf-ubuntu-22.04-gcc.tar.xz -o riscv64-elf-ubuntu-22.04-gcc.tar.xz \
 && tar -xJvf riscv64-elf-ubuntu-22.04-gcc.tar.xz \
 && rm riscv64-elf-ubuntu-22.04-gcc.tar.xz

# Add RISC-V toolchain to PATH
ENV PATH="/opt/riscv/bin:${PATH}"

WORKDIR /Lrix

# Copy current directory into the image and clean up
COPY . .
RUN rm Dockerfile

# Default to an interactive shell
CMD ["/bin/bash"]
