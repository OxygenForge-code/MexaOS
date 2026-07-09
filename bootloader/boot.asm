; ============================================================
;  MexaOS Bootloader — Stage 1
;  BIOS-based boot sector that loads Stage 2 and the Kernel
;  Architecture: x86_64 (starts in 16-bit real mode)
; ============================================================
;  Memory Layout:
;    0x7C00   — Boot sector loaded by BIOS
;    0x7E00   — Stage 2 bootloader
;    0x9000   — Kernel load address (32-bit protected mode)
;    0x100000 — Kernel high address (64-bit long mode)
; ============================================================

BITS 16
ORG 0x7C00

; ─── Configuration ───
%define STAGE2_SECTOR    2       ; LBA of stage 2 on disk
%define STAGE2_SIZE      16      ; Sectors to load for stage 2
%define KERNEL_SECTOR    18      ; LBA of kernel on disk
%define KERNEL_SIZE      128     ; Sectors to load for kernel
%define STAGE2_LOAD_ADDR 0x7E00
%define KERNEL_LOAD_ADDR 0x9000

; ─── Entry Point ───
_start:
    ; Disable interrupts during setup
    cli
    
    ; Zero segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Set up stack at 0x7C00 (grows downward)
    mov sp, 0x7C00
    
    ; Save boot drive number
    mov [boot_drive], dl
    
    ; Enable interrupts
    sti
    
    ; Print boot message
    mov si, msg_boot
    call print_string
    
    ; ─── Reset disk system ───
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    
    ; ─── Load Stage 2 ───
    mov si, msg_stage2
    call print_string
    
    mov ax, STAGE2_LOAD_ADDR / 16  ; Segment
    mov es, ax
    xor bx, bx                      ; Offset
    
    mov ah, 0x02                    ; Read sectors
    mov al, STAGE2_SIZE             ; Sector count
    mov ch, 0                       ; Cylinder 0
    mov cl, STAGE2_SECTOR + 1       ; Sector number (1-based)
    mov dh, 0                       ; Head 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    
    ; Verify stage 2 loaded
    cmp al, STAGE2_SIZE
    jne disk_error
    
    ; Jump to Stage 2
    mov si, msg_ok
    call print_string
    jmp STAGE2_LOAD_ADDR

; ─── Print String (SI = string address) ───
print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    jmp .loop
.done:
    popa
    ret

; ─── Disk Error Handler ───
disk_error:
    mov si, msg_disk_error
    call print_string
    ; Wait for keypress then reboot
    xor ah, ah
    int 0x16
    jmp 0xFFFF:0x0000               ; Reboot

; ─── Data ───
boot_drive:     db 0
msg_boot:       db "MexaOS Bootloader v1.0", 13, 10, 0
msg_stage2:     db "Loading Stage 2... ", 0
msg_ok:         db "OK", 13, 10, 0
msg_disk_error: db "Disk Error! Press any key to reboot...", 13, 10, 0

; ─── Boot Signature ───
times 510-($-$$) db 0
dw 0xAA55
