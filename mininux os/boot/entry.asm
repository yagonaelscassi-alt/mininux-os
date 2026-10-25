BITS 32

%define MB2_MAGIC 0xE85250D6
%define MB2_ARCH 0

section .multiboot
align 8
mb2_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd mb2_header_end - mb2_header_start
    dd -(MB2_MAGIC + MB2_ARCH + (mb2_header_end - mb2_header_start))
    dw 5
    dw 1
    dd 20
    dd 1024
    dd 768
    dd 32
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .text
global start
extern kernel_main

start:
    cli
    cld
    mov esp, stack_top
    mov [boot_magic], eax
    mov [boot_info], ebx

    mov edi, pml4_table
    xor eax, eax
    mov ecx, (4096 * 6) / 4
    rep stosd

    mov eax, pdpt_table
    or eax, 0x03
    mov [pml4_table], eax

    mov eax, pd_table
    or eax, 0x03
    mov [pdpt_table], eax
    mov eax, pd_table_1
    or eax, 0x03
    mov [pdpt_table + 8], eax
    mov eax, pd_table_2
    or eax, 0x03
    mov [pdpt_table + 16], eax
    mov eax, pd_table_3
    or eax, 0x03
    mov [pdpt_table + 24], eax

    mov eax, 0x83
    mov edi, pd_table
    call map_2mb_page_table
    mov edi, pd_table_1
    call map_2mb_page_table
    mov edi, pd_table_2
    call map_2mb_page_table
    mov edi, pd_table_3
    call map_2mb_page_table

    lgdt [gdt64.pointer]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, pml4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x08:long_mode_start

map_2mb_page_table:
    mov ecx, 512
.map_2mb_pages:
    mov [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .map_2mb_pages
    ret

BITS 64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    lea rsp, [rel stack_top]
    xor rbp, rbp

    mov edi, [rel boot_info]
    mov esi, [rel boot_magic]
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt

section .rodata
align 8
gdt64:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
.pointer:
    dw $ - gdt64 - 1
    dd gdt64

section .bss
alignb 8
boot_magic:
    resd 1
boot_info:
    resd 1

alignb 4096
pml4_table:
    resb 4096
pdpt_table:
    resb 4096
pd_table:
    resb 4096
pd_table_1:
    resb 4096
pd_table_2:
    resb 4096
pd_table_3:
    resb 4096

alignb 16
stack_bottom:
    resb 16384
stack_top:
