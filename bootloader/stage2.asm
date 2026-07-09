; ============================================================
;  MexaOS Bootloader — Stage 2
;  Transitions: Real Mode → Protected Mode → Long Mode (x86_64)
;  Loads the kernel into high memory and jumps to it
; ============================================================

BITS 16
ORG 0x7E00

; ─── Configuration ───
%define KERNEL_SECTOR       18
%define KERNEL_SIZE         128
%define KERNEL_LOAD_LOW     0x9000
%define KERNEL_LOAD_HIGH    0x100000
%define PAGE_TABLE_ADDR     0x8000

; ─── Stage 2 Entry ───
stage2_entry:
    ; Print stage 2 message
    mov si, msg_stage2
    call print16
    
    ; ─── Enable A20 Line ───
    call enable_a20
    
    ; ─── Get memory map using BIOS E820 ───
    call detect_memory
    
    ; ─── Load kernel to low memory first ───
    mov si, msg_kernel
    call print16
    
    mov ax, KERNEL_LOAD_LOW / 16
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02
    mov al, KERNEL_SIZE
    mov ch, 0
    mov cl, KERNEL_SECTOR + 1
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error16
    
    mov si, msg_ok
    call print16
    
    ; ─── Enter Protected Mode ───
    mov si, msg_prot
    call print16
    
    ; Load GDT
    cli
    lgdt [gdt_descriptor]
    
    ; Enable Protected Mode (PE bit in CR0)
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to 32-bit code
    jmp CODE_SEG:protected_mode

; ─── 16-bit A20 Enable ───
enable_a20:
    pusha
    
    ; Try BIOS method first
    mov ax, 0x2401
    int 0x15
    jc .try_keyboard
    test ah, ah
    jz .done
    
.try_keyboard:
    ; Keyboard controller method
    call .wait_cmd
    mov al, 0xAD
    out 0x64, al
    call .wait_cmd
    mov al, 0xD0
    out 0x64, al
    call .wait_data
    in al, 0x60
    push ax
    call .wait_cmd
    mov al, 0xD1
    out 0x64, al
    call .wait_cmd
    pop ax
    or al, 2
    out 0x60, al
    call .wait_cmd
    mov al, 0xAE
    out 0x64, al
    call .wait_cmd
    
.done:
    popa
    ret

.wait_cmd:
    in al, 0x64
    test al, 2
    jnz .wait_cmd
    ret

.wait_data:
    in al, 0x64
    test al, 1
    jz .wait_data
    ret

; ─── BIOS E820 Memory Detection ───
detect_memory:
    pusha
    mov si, msg_mmap
    call print16
    
    xor ebx, ebx
    mov edx, 0x534D4150         ; 'SMAP' signature
    mov di, memory_map_buffer
    
.e820_loop:
    mov eax, 0xE820
    mov ecx, 24                 ; Entry size
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done
    
    test ebx, ebx
    jz .done
    
    add di, 24
    cmp di, memory_map_buffer + 512
    jb .e820_loop
    
.done:
    mov si, msg_ok
    call print16
    popa
    ret

; ─── 16-bit Print ───
print16:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

disk_error16:
    mov si, msg_disk_err
    call print16
    jmp $

; ─── GDT (Global Descriptor Table) ───
gdt_start:
    dq 0                        ; Null descriptor

; Code segment (32-bit)
gdt_code:
    dw 0xFFFF                   ; Limit
    dw 0                        ; Base
    db 0
    db 10011010b                ; Access: present, ring 0, code, execute/read
    db 11001111b                ; Flags: 4KB granularity, 32-bit
    db 0

; Data segment (32-bit)
gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b                ; Access: present, ring 0, data, read/write
    db 11001111b
    db 0

; Code segment (64-bit)
gdt_code64:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 10101111b                ; Flags: long mode
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

%define CODE_SEG gdt_code - gdt_start
%define DATA_SEG gdt_data - gdt_start
%define CODE64_SEG gdt_code64 - gdt_start

; ─── Data ───
boot_drive:      db 0
msg_stage2:      db "Stage 2 loaded. Enabling A20... ", 0
msg_mmap:        db "Detecting memory... ", 0
msg_kernel:      db "Loading kernel... ", 0
msg_prot:        db "Entering Protected Mode...", 13, 10, 0
msg_disk_err:    db "Disk error!", 13, 10, 0
msg_ok:          db "OK", 13, 10, 0
memory_map_buffer: times 512 db 0

; ─── 32-bit Protected Mode ───
BITS 32

protected_mode:
    ; Set up data segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, 0x90000
    
    ; Print protected mode message
    mov esi, msg_prot_ok
    call print32
    
    ; ─── Copy kernel to high memory (0x100000) ───
    mov esi, msg_copy
    call print32
    
    mov esi, KERNEL_LOAD_LOW
    mov edi, KERNEL_LOAD_HIGH
    mov ecx, KERNEL_SIZE * 512 / 4
    rep movsd
    
    ; ─── Set up paging for Long Mode ───
    call setup_paging
    
    ; ─── Enable PAE ───
    mov eax, cr4
    or eax, 1 << 5              ; PAE bit
    mov cr4, eax
    
    ; ─── Set LM-bit in EFER MSR ───
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8              ; LME bit
    wrmsr
    
    ; ─── Enable paging ───
    mov eax, cr0
    or eax, 1 << 31             ; PG bit
    mov cr0, eax
    
    ; ─── Jump to Long Mode ───
    jmp CODE64_SEG:long_mode

; ─── Setup Paging ───
setup_paging:
    ; Clear page tables
    mov edi, PAGE_TABLE_ADDR
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; Set up PML4
    mov edi, PAGE_TABLE_ADDR
    mov dword [edi], PAGE_TABLE_ADDR + 0x1003  ; PDPT
    
    ; Set up PDPT
    mov edi, PAGE_TABLE_ADDR + 0x1000
    mov dword [edi], PAGE_TABLE_ADDR + 0x2003  ; PDT
    
    ; Set up PDT (maps first 2MB)
    mov edi, PAGE_TABLE_ADDR + 0x2000
    mov dword [edi], 0x83       ; 2MB page, present, writable
    
    ret

; ─── 32-bit Print ───
print32:
    pusha
    mov edx, 0xB8000
.loop:
    lodsb
    test al, al
    jz .done
    mov [edx], al
    mov byte [edx + 1], 0x0F    ; White on black
    add edx, 2
    jmp .loop
.done:
    popa
    ret

msg_prot_ok: db "Protected Mode active.", 0
msg_copy:    db "Copying kernel to high memory... ", 0

; ─── 64-bit Long Mode ───
BITS 64

long_mode:
    ; Set up 64-bit segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up 64-bit stack
    mov rsp, 0x200000
    
    ; Print long mode message
    mov rsi, msg_long
    call print64
    
    ; Pass memory map info to kernel
    mov rdi, memory_map_buffer
    
    ; Jump to kernel!
    mov rsi, msg_jump
    call print64
    
    mov rax, KERNEL_LOAD_HIGH
    jmp rax

; ─── 64-bit Print (simple VGA) ───
print64:
    push rax
    push rdx
    push rsi
    mov rdx, 0xB8000
.loop:
    lodsb
    test al, al
    jz .done
    mov [rdx], al
    mov byte [rdx + 1], 0x1F    ; White on blue (MexaOS theme)
    add rdx, 2
    jmp .loop
.done:
    pop rsi
    pop rdx
    pop rax
    ret

msg_long: db "Long Mode (x86_64) active! MexaOS kernel loading...", 0
msg_jump: db "Jumping to kernel...", 0

times 4096-($-$$) db 0          ; Pad to 4KB (8 sectors)
