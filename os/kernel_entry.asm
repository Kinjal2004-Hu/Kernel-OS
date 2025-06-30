; kernel_entry.asm  ── 32-bit, Multiboot v1
; -------------------------------------------------
;  • Adds the required Multiboot header (first 8 KiB of file).
;  • Sets up a 16 KiB stack.
;  • Passes the Multiboot info-pointer (in EBX) to kmain().
; -------------------------------------------------

[bits 32]

; ───────────────────────────────────────────────────
; Multiboot header  (must be 4-byte aligned)
; ───────────────────────────────────────────────────
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002          ; magic
    dd 0x00000000          ; flags  (0 = simplest)
    dd 0xE4524FFE          ; checksum = -(magic + flags)

; ───────────────────────────────────────────────────
section .text
global _start
extern  kmain

_start:
    ; 1. set up the stack  (top-down)
    mov     esp, stack_top

    ; 2. pass Multiboot structure pointer (in EBX) to C kernel
    push    ebx            ; arg0 = multiboot_info*
    call    kmain
    add     esp, 4         ; clean stack

.halt:
    cli
    hlt
    jmp     .halt          ; never returns

; ───────────────────────────────────────────────────
; 16 KiB stack (BSS — zero-filled by the linker)
; ───────────────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb    16384          ; 16 KiB
stack_top:
