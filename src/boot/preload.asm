[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [BOOT_DRIVE], dl

    ; Setup LBA transfer
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, lba_packet
    int 0x13
    jc disk_error

    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp 0x08:pm_entry

disk_error:
    jmp $

BOOT_DRIVE db 0

align 4
lba_packet:
    db 0x10
    db 0
    dw 128
    dw 0x0000
    dw 0x1000
    dq 1

align 8
gdt_start:
    dd 0, 0
gdt_code:
    dw 0xFFFF, 0
    db 0, 10011010b, 11001111b, 0
gdt_data:
    dw 0xFFFF, 0
    db 0, 10010010b, 11001111b, 0
gdt_descriptor:
    dw gdt_descriptor - gdt_start - 1
    dd gdt_start

[BITS 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp
    jmp 0x08:0x10000

times 510-($-$$) db 0
dw 0xAA55