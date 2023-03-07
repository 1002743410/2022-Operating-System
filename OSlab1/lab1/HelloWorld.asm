;Section to store uninitialized variables
section .data
string: db 'Hello World', 0Ah
length: equ 13
section .bss
var: resb 1

section .text
global _start:
_start:
mov eax, 4
mov ebx, 1
mov ecx, string
mov edx, length
int 80h
;System Call to exit
mov eax, 1
mov ebx, 0
int 80h
