[org 0x7c00]
[bits 16]
start:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00
   
    ; Save boot drive
    mov [boot_drive], dl
   
    ; Print "Loading kernel..."
    mov si, loading_message
    call print_string
   
    ; Reset Disk
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
   
    ; Print "Disk reset OK"
    mov si, reset_ok_message
    call print_string
   
    ; Read kernel (next 15 sectors) into 0x1000:0000
    mov ah, 0x02
    mov al, 15               ; Increased to 15 sectors
    mov ch, 0
    mov cl, 2                ; Start at sector 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000           ; Load segment into BX first
    mov es, bx               ; Then move to ES
    mov bx, 0x0000           ; Offset
    int 0x13
    jc disk_error
   
    ; Print "Kernel loaded"
    mov si, kernel_loaded_message
    call print_string
   
    ; Setup GDT
    lgdt [gdt_descriptor]
   
    ; Print "Entering protected mode"
    mov si, protected_mode_message
    call print_string
   
    ; Enter Protected Mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

disk_error:
    mov si, disk_error_message
    call print_string
    jmp $

print_string:
    mov ah, 0x0E
.next_char:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .next_char
.done:
    ret

loading_message db "Loading kernel...", 0x0D, 0x0A, 0
reset_ok_message db "Disk reset OK", 0x0D, 0x0A, 0
kernel_loaded_message db "Kernel loaded into memory", 0x0D, 0x0A, 0
protected_mode_message db "Entering protected mode...", 0x0D, 0x0A, 0
disk_error_message db "Disk read error! System halted.", 0x0D, 0x0A, 0
boot_drive db 0

gdt:
    dq 0x0000000000000000      ; Null
    dq 0x00CF9A000000FFFF      ; Code
    dq 0x00CF92000000FFFF      ; Data

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt

gdt_end:

times 510-($-$$) db 0
dw 0xAA55

[bits 32]
protected_mode:
    mov ax, 0x10               ; Data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
   
    ; Clear screen in protected mode
    mov edi, 0xb8000
    mov ecx, 2000              ; 80x25 screen
    mov ax, 0x0720             ; Space character with white on black
    rep stosw
   
    jmp 0x08:0x10000           ; Jump to kernel