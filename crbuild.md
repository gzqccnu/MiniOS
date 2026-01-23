# Build docker image

## 🚀 Quick start
```bash
git clone https://github.com/lrisguan/Lrix.git
sudo docker build -t lrix .
```
Waiting for minutes. The building process may take a long time. Please be patient.

> [!Tip]
> If you are in China, download riscv-gnu-toochain in docker is hard. You could 
> download it via VPN in your host OS first and then modify [Dockerfile](./Dockerfile) 
> copying the archive package into image like this:
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
