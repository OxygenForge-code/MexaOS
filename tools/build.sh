#!/bin/bash
# ============================================================
#  MexaOS Build Script
#  Automated build with dependency checking
# ============================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

VERSION="2.0.0"
BUILD="2240"
CODENAME="Intent"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
ISO_DIR="$PROJECT_DIR/iso"

echo -e "${MAGENTA}"
echo "  __  ___                      ____   _____"
echo "  /  |/  /____ _   _____  _____/ __ \/ ___/"
echo " / /|_/ // __ \\ | / / _ \\/ ___/ / / /\\__ \\"
echo "/ /  / // /_/ / |/ /  __/ /  / /_/ /___/ /"
echo "/_/  /_/ \\____/|___/\\___/_/   \\____//____/"
echo ""
echo -e "${CYAN}  Intent-Driven Operating System v${VERSION}${NC}"
echo -e "${CYAN}  Build ${BUILD} — Codename: \"${CODENAME}\"${NC}"
echo ""

echo -e "${BLUE}[CHECK] Checking dependencies...${NC}"
MISSING=0

check_tool() {
    if command -v "$1" &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $1 found"
        return 0
    else
        echo -e "  ${RED}✗${NC} $1 NOT FOUND"
        MISSING=1
        return 1
    fi
}

check_tool "nasm"
check_tool "qemu-system-x86_64"

if command -v "x86_64-elf-gcc" &> /dev/null; then
    echo -e "  ${GREEN}✓${NC} x86_64-elf-gcc found"
    export CC=x86_64-elf-gcc
    export LD=x86_64-elf-ld
    export OBJCOPY=x86_64-elf-objcopy
elif command -v "gcc" &> /dev/null; then
    echo -e "  ${YELLOW}!${NC} x86_64-elf-gcc not found, using system gcc"
    export CC=gcc
    export LD=ld
    export OBJCOPY=objcopy
else
    echo -e "  ${RED}✗${NC} No C compiler found"
    MISSING=1
fi

if [ $MISSING -eq 1 ]; then
    echo ""
    echo -e "${RED}[ERROR] Missing dependencies. Please install:${NC}"
    echo "  • nasm (Netwide Assembler)"
    echo "  • x86_64-elf-gcc (Cross compiler) or gcc"
    echo "  • qemu-system-x86_64 (for testing)"
    echo "  • grub2-mkrescue (for ISO creation)"
    exit 1
fi

echo ""
echo -e "${BLUE}[BUILD] Creating build directories...${NC}"
mkdir -p "$BUILD_DIR"
mkdir -p "$ISO_DIR/boot/grub"

echo ""
echo -e "${BLUE}[BUILD] Building MexaOS...${NC}"
cd "$PROJECT_DIR"
make clean 2>/dev/null || true
make all

if [ -f "$BUILD_DIR/mexaos.bin" ]; then
    echo ""
    echo -e "${GREEN}[SUCCESS] MexaOS built successfully!${NC}"
    echo ""
    echo -e "  Kernel binary: ${CYAN}$BUILD_DIR/mexaos.bin${NC}"
    echo -e "  ISO image:     ${CYAN}$BUILD_DIR/mexaos.iso${NC}"
    echo ""
    echo -e "  ${YELLOW}Run with:${NC} make run"
    echo -e "  ${YELLOW}Debug with:${NC} make debug"
    echo ""
else
    echo ""
    echo -e "${RED}[ERROR] Build failed!${NC}"
    exit 1
fi
