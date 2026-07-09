# MexaOS Build Guide

## Prerequisites

To build MexaOS from source, you need the following tools installed:

### Required Packages

```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install -y build-essential nasm xorriso grub-pc-bin qemu-system-x86

# Arch Linux
sudo pacman -S base-devel nasm xorriso grub qemu-full

# macOS (with Homebrew)
brew install nasm xorriso qemu
# Note: macOS requires a Linux cross-compiler or Docker
```

### Cross-Compiler Toolchain

MexaOS requires an `x86_64-elf` cross-compiler. You have several options:

#### Option 1: Pre-built Toolchain (Recommended)

```bash
wget https://newos.org/toolchains/x86_64-elf-7.5.0-Linux-x86_64.tar.xz
tar -xf x86_64-elf-7.5.0-Linux-x86_64.tar.xz -C /usr/local --strip-components=1
```

#### Option 2: Build from Source

```bash
wget https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz

tar -xf binutils-2.40.tar.xz
mkdir build-binutils && cd build-binutils
../binutils-2.40/configure --target=x86_64-elf --prefix=/usr/local --disable-nls --disable-werror
make -j$(nproc) && sudo make install

tar -xf gcc-13.2.0.tar.xz
mkdir build-gcc && cd build-gcc
../gcc-13.2.0/configure --target=x86_64-elf --prefix=/usr/local --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc all-target-libgcc && sudo make install-gcc install-target-libgcc
```

#### Option 3: Docker (Easiest)

```bash
docker run --rm -v $(pwd):/src nativeos/cross-compiler make -C /src all
```

## Building

```bash
git clone https://github.com/OxygenForge-code/MexaOS.git
cd MexaOS
make all
```

| Target | Description |
|--------|-------------|
| `make all` | Build complete ISO image |
| `make run` | Build and run in QEMU |
| `make debug` | Run with GDB debugging support |
| `make clean` | Remove all build artifacts |

## Running in QEMU

```bash
qemu-system-x86_64 -m 512M -cdrom build/mexaos.iso -vga std -display sdl
```

## New Files

- **kernel/shell.c** - MexaShell CLI with 24 commands, AI mode, tab completion
- **kernel/gui.c** - Framebuffer graphics with MexaOS indigo theme, desktop, taskbar
- **kernel/window.c** - Compositing window manager with glass effects

## Automated Builds

GitHub Actions is configured to automatically build ISO on every push.
Download the latest ISO from the [Releases](../../releases) page.
