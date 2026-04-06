section .multiboot_header
header_start:
    dd 0xe85250d6                ; magic number (multiboot 2)
    dd 0                         ; architecture 0 (protected mode i386)
    dd header_end - header_start ; header length
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    align 8
    ; information request tag
    dw 1    ; type
    dw 0    ; flags
    dd 24   ; size (tag size including header + list of mmaped types)
    dd 1    ; request command line
    dd 3    ; request modules
    dd 6    ; request memory map
    dd 8    ; request framebuffer
    align 8

    ; framebuffer tag
    dw 5    ; type
    dw 0    ; flags
    dd 20   ; size
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth
    align 8

    ; end tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:
