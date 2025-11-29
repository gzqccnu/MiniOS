# MiniOS

## 📌 Introduction
This is a scratch implemention of a Operating System based on [RISC-V](https://riscv.org).

## 🚀 Quick start
To run this project, you need to have belowings:
### Dependencies
- **RISC-V toolchain**
    for compiling C programs.
    #### Installation steps
    
    > [!Warning]
    > git clone takes around 6.65 GB of disk and download size
    ##### Prerequisites
    - On **Ubuntu/debain**
    ```bash
    sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev
    ```
    - On **Fedora/CentOS/RHEL OS**
    ```bash
    sudo yum install autoconf automake python3 libmpc-devel mpfr-devel gmp-devel gawk  bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel libslirp-devel
    ```
    - On **Arch Linux**
    ```bash
    sudo pacman -Syu curl python3 libmpc mpfr gmp base-devel texinfo gperf patchutils bc zlib expat libslirp
    ```
    ##### Installation
    ```bash
    git clone https://github.com/riscv/riscv-gnu-toolchain
    ./configure --prefix=/opt/riscv
    make -j$(nproc)
    echo 'export PATH="/opt/riscv/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    ```
    Then you can execute command:
    ```bash
    riscv64-unknown-elf-gcc --version
    ```
    to verify whether the toolchain is installed.
- **qemu-system-riscv**
    for create a virtual machine of RISC-V.
    #### Installation steps
    ##### Install from prebuild package
    - On **Ubuntu/Debain**
    ```bash
    sudo apt install qemu qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils 
    sudo systemctl enable --now libvirtd
    ```
    - On **Fedora/CentOS/RHEL OS**
    ```bash
    sudo yum install -y qemu-kvm libvirt libvirt-python libguestfs-tools virt-install virt-manager
    # or
    sudo dnf install -y qemu-kvm libvirt libvirt-client virt-install virt-manager virt-top libguestfs-tools virt-viewer
    # then
    sudo systemctl enable --now libvirtd
    ```
    - On **Arch Linux**
    ```bash
    sudo pacman -S qemu-full qemu-img libvirt virt-install virt-manager virt-viewer dk2-ovmf dnsmasq swtpm guestfs-tools libosinfo tuned
    sudo systemctl enable --now libvirtd
    ```
    ##### Install from souce
    
    > [!Tip]
    > If you install qemu from source, may you need a higher version of **glibc**. When I compile it, the version is **2.35** at least. Considering that **glibc** is a very import system dependency, I recommand to compile **qemu** in docker instead. You can see the fowlling **docker(optional)** part.
    ```bash
    git clone https://gitlab.com/qemu-project/qemu.git
    mkdir build
    cd build
    ../configure
    make -j$(nproc)
    sudo make install
    ```
- **docker(optional)** for a higher version of **glibc** to compile qemu.
    > Only when you compile the **latest qemu** from source, you may need to install docker for a higher version of **glibc**.

    > [!Important]
    > Ensure your OS has a higher version of **glibc** then continue reading the rest of the content
    Compiling steps
    ```bash
    git clone https://gitlab.com/qemu-project/qemu.git
    mkdir build
    cd build
    ../configure
    make -j$(nproc)
    sudo make install
    ```
### Run
- Clone this repo
    ```bash
    git clone https://github.com/gzqccnu/MiniOS.git
    cd Minios
    ```
- **kernel**

    run kernel only
    ```bash
    cd kernel
    make
    make run
    ```
    Additionally, you can compile and run the kernel for only selected units. For now, you can run:
    ```bash
    make DIR="boot mem uart trap string"
    make DIR="boot mem uart trap string" run
    ```
- **usr**
TBA
- **whole os**
TBA

