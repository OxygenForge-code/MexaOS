# MexaOS

## Intent-Driven Operating System v2.0.0

**MexaOS** is a real, from-scratch operating system for x86_64. It's not a simulation — it's a bare-metal OS with its own bootloader, kernel, file system, AI engine, and GUI.

### What's New in v2.0

- **AI Intent Engine** — Control your OS with natural language
- **.mex File System** — Native filesystem with AI metadata
- **5 Intent Workspaces** — Main, Coding, Creative, Focus, Social
- **MexaOS Indigo Theme** — Aurora glass morphism UI
- **Complete x86_64 Kernel** — Paging, multitasking, memory management

### Architecture

```
bootloader/     x86_64 bootloader (real mode → long mode)
kernel/         Monolithic kernel + all subsystems
├── kmain.c     Kernel entry point
├── vga.c       Display driver (text + graphics)
├── interrupt.c IDT/PIC interrupt handling
├── memory.c    PMM + VMM paging + heap allocator
├── timer.c     1ms PIT timer with callbacks
├── keyboard.c  PS/2 keyboard + MexaOS special keys
├── serial.c    COM1 debug serial port
├── process.c   Round-robin priority scheduler
├── mexfs.c     .mex native file system
├── ai_engine.c AI Intent Engine (core feature)
├── shell.c     MexaShell command interface
├── gui.c       Framebuffer graphics engine
├── window.c    Compositing window manager
└── isr.asm     Assembly ISR/IRQ stubs

include/        Core headers
Makefile        Complete build system
```

### Building

```bash
# Requirements: nasm, x86_64-elf-gcc, qemu-system-x86_64

make          # Build kernel binary and ISO
make run      # Build and run in QEMU
make debug    # Debug mode with GDB
```

### Key Features

| Feature | Description |
|---------|-------------|
| AI Intents | `"Open browser"`, `"Focus mode"`, `"Prepare for coding"` |
| Meta+Space | Global AI intent shortcut |
| .mex Files | Native format with embedded intent metadata |
| 5 Workspaces | Context-aware desktop environments |
| Glass UI | Aurora-themed translucent interface |

### Documentation

See [README_OS.md](README_OS.md) for full technical documentation.

---

*MexaOS — An OS That Understands You.*
