# 构建 Docker 镜像
## 🚀 快速开始
```bash
git clone https://github.com/lrisguan/Lrix.git
sudo docker build -t lrix .
```
请耐心等待几分钟。构建过程可能需要较长时间，请保持耐心。
> [!Tip]
> 如果您在中国，在 Docker 中下载 RISC-V GNU 工具链可能会比较困难。您可以先在宿主操作系统中通过 VPN 下载，然后修改 [Dockerfile](./Dockerfile)，将压缩包复制到镜像中，如下所示：
> ```Dockerfile
> FROM ubuntu:22.04
> 
> ENV DEBIAN_FRONTEND=noninteractive
> 
> RUN apt-get update && apt-get install -y \
>     ca-certificates \
>     curl \
>     xz-utils \
>     make \
>     python3 \
>     qemu-system-riscv64 \
>     libmpc3 \
>     libmpfr6 \
>     libgmp10 \
>     && rm -rf /var/lib/apt/lists/*
> 
> RUN mkdir -p /opt \
>     && cd /opt
>
> COPY riscv64-elf-ubuntu-22.04-gcc.tar.xz .
> 
> RUN tar -xJvf riscv64-elf-ubuntu-22.04-gcc.tar.xz \
>     && rm riscv64-elf-ubuntu-22.04-gcc.tar.xz
> 
> ENV PATH="/opt/riscv/bin:${PATH}"
> 
> WORKDIR /Lrix
> 
> COPY . .
>
> RUN rm Dockerfile
> 
> CMD ["/bin/bash"]
> 
> ```
