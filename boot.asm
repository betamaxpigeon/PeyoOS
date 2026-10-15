bits 32

section .multiboot
align 8
multiboot_start:
  dd 0xe85250d6
  dd 0
  dd multiboot_end - multiboot_start
  dd -(0xe85250d6 + (multiboot_end - multiboot_start))

align 8
  dw 5
  dw 0
  dd 20
  dd 1024
  dd 768
  dd 32

align 8
  dw 0
  dd 8
multiboot_end:

section .text
global _start
extern kernel_main

_start:
  cli
  mov esp, stack_top
  mov [multiboot_magic], eax
  mov [multiboot_info], ebx

  mov eax, 0x80000000
  cpuid
  cmp eax, 0x80000001
  jb boot_halt
  mov eax, 0x80000001
  cpuid
  test edx, 1 << 29
  jz boot_halt

  lgdt [gdt_descriptor]

  mov eax, pdpt_table
  or eax, 0x3
  mov [pml4_table], eax
  mov eax, page_directories
  or eax, 0x3
  mov [pdpt_table], eax
  mov eax, page_directories + 4096
  or eax, 0x3
  mov [pdpt_table + 8], eax
  mov eax, page_directories + 8192
  or eax, 0x3
  mov [pdpt_table + 16], eax
  mov eax, page_directories + 12288
  or eax, 0x3
  mov [pdpt_table + 24], eax

  mov edi, page_directories
  mov eax, 0x83
  mov ecx, 2048
map_pages:
  mov [edi], eax
  add edi, 8
  add eax, 0x200000
  loop map_pages

  mov eax, cr4
  or eax, 0x30
  mov cr4, eax
  mov eax, pml4_table
  mov cr3, eax

  mov ecx, 0xc0000080
  rdmsr
  or eax, 1 << 8
  wrmsr

  mov eax, cr0
  or eax, 1 << 31
  mov cr0, eax
  jmp 0x08:long_mode_start

boot_halt:
  cli
  hlt
  jmp boot_halt

bits 64
long_mode_start:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov ss, ax
  xor eax, eax
  mov fs, ax
  mov gs, ax
  cld

  lea rsp, [rel stack_top]
  and rsp, -16
  mov edi, [rel multiboot_magic]
  mov esi, [rel multiboot_info]
  call kernel_main

long_mode_halt:
  cli
  hlt
  jmp long_mode_halt

section .rodata
align 8
gdt_start:
  dq 0
  dq 0x00af9a000000ffff
  dq 0x00cf92000000ffff
gdt_end:

gdt_descriptor:
  dw gdt_end - gdt_start - 1
  dd gdt_start

section .bss
align 4096
pml4_table:
  resq 512
pdpt_table:
  resq 512
page_directories:
  resq 2048

align 16
stack_bottom:
  resb 16384
stack_top:

multiboot_magic:
  resd 1
multiboot_info:
  resd 1
