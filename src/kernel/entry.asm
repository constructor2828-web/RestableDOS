global _start
extern _start_64

section .text
[bits 32]
_start:
    ; Terminate if not multiboot
    cmp eax, 0x36d76289
    jne .halt

    ; Save multiboot info
    mov edi, ebx

    ; Set up stack
    mov esp, stack_top

    ; Check for long mode support
    call check_cpuid
    call check_long_mode

    ; Set up paging
    call setup_page_tables
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:_start_64

.halt:
    cli
    hlt
    jmp .halt

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    jmp .halt

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    jmp .halt

setup_page_tables:
    ; 1. Clear page tables memory
    mov edi, page_table_l4
    xor eax, eax
    mov ecx, 4096 * 3 / 4 ; L4, L3, L2
    rep stosd

    ; 2. Map PML4 to PDP
    mov eax, page_table_l3
    or eax, 0b11 ; present, writable
    mov [page_table_l4], eax

    ; 3. Map PDP to PD
    mov eax, page_table_l2
    or eax, 0b11 ; present, writable
    mov [page_table_l3], eax

    ; 4. Map PD to 2MiB Huge Pages (Identity map first 1GB)
    mov ecx, 0
.loop:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011 ; present, writable, huge
    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .loop
    ret

enable_paging:
    ; Load PML4 to CR3
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set long mode bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

[bits 64]
extern kernel_main
_start_64:
    ; Load 64-bit data segments
    mov ax, gdt64.data_segment
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; EDI still contains multiboot info pointer
    mov rdi, rdi ; RDI = Multiboot Info
    call kernel_main

    cli
    hlt

section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096
stack_bottom:
    resb 4096 * 4
stack_top:

section .rodata
gdt64:
    dq 0 ; zero entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.data_segment: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41) ; data segment (present, writable, data)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
