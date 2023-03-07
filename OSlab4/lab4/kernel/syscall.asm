
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
INT_VECTOR_SYS_CALL equ 0x90 ; Linux相应的中断号是0x80，只要不和原来的中断号重复即可

; 添加系统调用的声明
_NR_sleep	    	equ 1
_NR_print  			equ 2
_NR_P      			equ 3
_NR_V       		equ 4

; 导出符号
global	get_ticks
global 	sleep
global 	print
global 	P
global 	V

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
; 系统调用
get_ticks:
	mov	eax, _NR_get_ticks ;知道问题是“请问当前的ticks是多少？”，eax存放问题
	int	INT_VECTOR_SYS_CALL
	ret

sleep:
	mov eax, _NR_sleep
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

print:
	mov eax, _NR_print
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

P:
	mov eax, _NR_P
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

V:
	mov eax, _NR_V
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret	
