# MexaOS — Technical Documentation

> **Intent-Driven Operating System**  
> Version 2.0.0 | Build 2240 | Codename: "Intent"

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Boot Process](#boot-process)
- [Kernel Subsystems](#kernel-subsystems)
- [Memory Management](#memory-management)
- [Process Scheduler](#process-scheduler)
- [.mex File System](#mex-file-system)
- [AI Intent Engine](#ai-intent-engine)
- [Shell (MexaShell)](#shell)
- [GUI Subsystem](#gui-subsystem)
- [Building & Running](#building--running)
- [System Requirements](#system-requirements)
- [API Reference](#api-reference)

---

## Architecture Overview

MexaOS is a 64-bit (x86_64) intent-driven operating system built from scratch. It features a monolithic kernel with the following architecture:

```
┌─────────────────────────────────────────────────────┐
│  User Space                                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │ MexaShell│ │  Apps    │ │   GUI    │ │ Window  │ │
│  │ (shell.c)│ │(proc.)   │ │ (gui.c)  │ │ Manager │ │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬────┘ │
├───────┼────────────┼────────────┼────────────┼──────┤
│       ▼            ▼            ▼            ▼       │
│  ┌─────────────────────────────────────────────────┐ │
│  │           AI Intent Engine (ai_engine.c)         │ │
│  │   Natural Language → Intent → System Action      │ │
│  └─────────────────────────────────────────────────┘ │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │  .mex FS │ │  Memory  │ │ Process  │ │  Timer  │ │
│  │(mexfs.c) │ │(memory.c)│ │(process.c)│ │(timer.c)│ │
│  └──────────┘ └──────────┘ └──────────┘ └─────────┘ │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │   VGA    │ │ Keyboard │ │   PIC    │ │  IDT    │ │
│  │ (vga.c)  │ │(keyboard.c)│ │(interrupt.c)│ │(isr.asm)│ │
│  └──────────┘ └──────────┘ └──────────┘ └─────────┘ │
├──────────────────────────────────────────────────────┤
│  Bootloader: boot.asm → stage2.asm → kmain()        │
└──────────────────────────────────────────────────────┘
```

---

## Boot Process

1. **BIOS** loads boot sector (512 bytes) from disk to `0x7C00`
2. **Stage 1** (`boot.asm`) loads Stage 2 from disk
3. **Stage 2** (`stage2.asm`):
   - Enables A20 line
   - Detects memory (E820)
   - Enters Protected Mode (32-bit)
   - Sets up paging for Long Mode
   - Enters Long Mode (64-bit)
   - Loads kernel to `0x100000`
4. **Kernel Entry** (`kmain.c`):
   - Parses Multiboot2 info
   - Initializes all subsystems
   - Starts scheduler and shell/GUI

---

## Kernel Subsystems

### Interrupt Handling (`interrupt.c`, `isr.asm`)
- 256-entry IDT for x86_64
- 32 CPU exceptions (0-31) with MexaOS-themed error screens
- 16 IRQ handlers (remapped to 0x20-0x2F)
- Syscall gate via `int 0x80`
- Full context save/restore in assembly

### Timer (`timer.c`)
- PIT-based 1ms tick (1000Hz)
- Callback registration system
- Sleep functions (ms and ticks)
- Uptime tracking

### Keyboard (`keyboard.c`)
- PS/2 keyboard driver
- US QWERTY layout
- Full modifier support (Shift, Ctrl, Alt, Meta)
- **MexaOS special keys:**
  - `Meta+Space` → AI Intent mode
  - `Meta+1..5` → Workspace switch
  - `Meta+Q` → Quick actions

### Serial Port (`serial.c`)
- COM1 115200 baud
- Kernel debug logging
- Non-blocking I/O

---

## Memory Management

### Physical Memory Manager (`memory.c`)
- Bitmap-based allocation
- Block size: 4KB (PAGE_SIZE)
- Functions: `pmm_alloc_block()`, `pmm_free_block()`

### Virtual Memory Manager (`memory.c`)
- x86_64 4-level paging (PML4 → PDPT → PD → PT)
- Higher half kernel mapping (`0xFFFFFFFF80000000`)
- Huge page support (2MB)

### Heap (`memory.c`)
- Bump allocator + slab allocator
- Sizes: 16, 32, 64, 128, 256, 512, 1024, 2048 bytes
- Functions: `kmalloc()`, `kzalloc()`, `kfree()`, `krealloc()`

---

## Process Scheduler

- Round-robin with 16 priority levels
- 256 max processes
- States: EMPTY, READY, RUNNING, BLOCKED, SLEEPING, ZOMBIE
- **Intent-driven process creation:**
  ```c
  pid_t process_create_from_intent("open browser for research", 8);
  ```
- Process attach intent for AI tracking

---

## .mex File System

MexaOS's native file system with AI metadata support.

### Structure
```
Superblock (4KB)
├── Magic: "MEXA" (0x4D455841)
├── Version: 1
├── Block Size: 4KB
├── Total Blocks: 65536 (256MB)
└── Max Inodes: 65536

Inode (256 bytes)
├── Standard: mode, uid, gid, size, blocks, timestamps
└── MexaOS-specific:
    ├── intent_tag[128]      — Associated intent
    ├── ai_summary[256]      — AI-generated description
    ├── ai_confidence        — Processing confidence (0-100)
    └── importance_score     — AI-calculated importance
```

### File Types
- `0` — Regular file
- `1` — Directory
- `5` — AI Model file
- `6` — Intent script

### Special Operations
```c
mex_set_intent("/docs/report.mex", "Quarterly business report");
mex_get_intent("/docs/report.mex", buffer, size);
mex_execute_intent("organize my desktop for coding");
```

---

## AI Intent Engine

The core of MexaOS. Converts natural language to system actions.

### Intent Categories
| Category | Keywords | Example |
|----------|----------|---------|
| OPEN | open, launch, start | "Open browser" |
| CLOSE | close, quit, exit | "Close terminal" |
| CREATE | create, new, make | "Create new document" |
| DELETE | delete, remove, trash | "Delete old files" |
| SEARCH | search, find, look | "Find my photos" |
| SYSTEM | system, status, info | "Show system info" |
| WORKSPACE | workspace, desktop | "Switch to coding" |
| FOCUS | focus, concentrate, quiet | "Enable focus mode" |
| CREATIVE | creative, design, art | "Open creative studio" |
| CODING | code, program, develop | "Prepare for coding" |

### Confidence Scoring
- Keyword matching (primary)
- Pattern learning (secondary)
- Context awareness (time-based)

### Workspace Integration
```
[1] Main      → General purpose
[2] Coding    → IDE layout, dark theme
[3] Creative  → Canvas, color tools
[4] Focus     → Distraction-free, quiet
[5] Social    → Chat, notifications on
```

---

## Shell (MexaShell)

Intent-aware command shell.

### Commands
| Command | Description | AI-Enabled |
|---------|-------------|------------|
| `help` | Show commands | ✦ |
| `intent` | Execute AI intent | ✦ |
| `ai` | Enter AI mode | ✦ |
| `ps` | List processes | ✦ |
| `mem` | Memory info | ✦ |
| `df` | Disk free | ✦ |
| `ws` | Workspace switch | ✦ |
| `mex` | .mex file ops | ✦ |
| `ls`, `cd`, `pwd` | File operations | ✦ |
| `sysinfo` | System info | ✦ |

### AI Mode
Press `Meta+Space` or type `ai` to enter AI intent mode. Type natural language descriptions.

---

## GUI Subsystem

### Features
- 32-bit ARGB framebuffer
- Primitive drawing: lines, rectangles, circles, triangles
- Text rendering with 8x8 font
- Gradient fills (vertical/horizontal)
- Blur and glow effects
- Glass morphism UI elements
- Mouse cursor

### MexaOS Theme
```
Background:  #050507 (deep black-blue)
Foreground:  #FFFFFF (white)
Accent:      #6366F1 (indigo)
Pink:        #EC4899
Cyan:        #06B6D4
Amber:       #F59E0B
```

### Window Manager
- Compositing window manager
- Glass effect windows with rounded corners
- Shadow effects
- Focus management
- Z-ordering (raise/lower)

---

## Building & Running

### Prerequisites
- `nasm` — Netwide Assembler
- `x86_64-elf-gcc` — Cross compiler (or system `gcc`)
- `qemu-system-x86_64` — For testing
- `grub2-mkrescue` — For ISO creation (optional)

### Build
```bash
# Quick build
make

# Build and run in QEMU
make run

# Debug mode (waits for GDB)
make debug

# Using build script
chmod +x tools/build.sh
./tools/build.sh
```

### Output
```
build/mexaos.bin    ← Raw kernel binary
build/mexaos.iso    ← Bootable ISO image
```

---

## System Requirements

### Minimum
- 8 MB RAM
- x86_64 CPU
- VGA-compatible display (text mode)
- PS/2 keyboard

### Recommended
- 32 MB RAM
- x86_64 multi-core CPU
- VESA framebuffer (for GUI)
- PS/2 or USB keyboard

---

## API Reference

### Kernel Functions
```c
// Memory
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *ptr);

// VGA
void vga_puts(const char *s);
void vga_putc(char c);
void vga_clear(void);

// Timer
uint64_t timer_get_ms(void);
void timer_sleep_ms(uint64_t ms);

// Process
pid_t process_create(const char *name, void (*entry)(void), uint8_t priority);
void process_yield(void);

// Filesystem
fid_t mex_open(const char *path, uint32_t mode);
ssize_t mex_read(fid_t fid, void *buf, size_t count);
ssize_t mex_write(fid_t fid, const void *buf, size_t count);

// AI
int ai_process_intent(const char *text);
int ai_classify_intent(const char *text);
```

---

## License

Copyright 2026 MexaOS Project. All rights reserved.

---

*MexaOS — An OS That Understands You.*
