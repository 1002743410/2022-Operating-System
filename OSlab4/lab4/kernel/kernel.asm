
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               kernel.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


%include "sconst.inc"

; 导入函数
extern	cstart
extern	kernel_main
extern	exception_handler
extern	spurious_irq
extern	clock_handler
extern	disp_str
extern	delay
extern	irq_table

; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	p_proc_ready
extern	tss
extern	disp_pos
extern	k_reenter
extern	sys_call_table ; sys_call_table是一个函数指针数组，每一个成员都指向一个函数，用以处理相应的系统调用

bits 32

[SECTION .data]
clock_int_msg		db	"^", 0

[SECTION .bss]
StackSpace		resb	2 * 1024
StackTop:		; 栈顶

[section .text]	; 代码在此

global _start	; 导出 _start

global restart
global sys_call

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15


_start:
	; 此时内存看上去是这样的（更详细的内存情况在 LOADER.ASM 中有说明）：
	;              ┃                                    ┃
	;              ┃                 ...                ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■Page  Tables■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■┃ PageTblBase
	;    00101000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■Page Directory Table■■■■┃ PageDirBase = 1M
	;    00100000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□ Hardware  Reserved □□□□┃ B8000h ← gs
	;       9FC00h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	;       90000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■KERNEL.BIN■■■■■■┃
	;       80000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	;       30000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┋                 ...                ┋
	;              ┋                                    ┋
	;           0h ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	; GDT 以及相应的描述符是这样的：
	;
	;		              Descriptors               Selectors
	;              ┏━━━━━━━━━━━━━━━━━━┓
	;              ┃         Dummy Descriptor           ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_C    (0～4G)     ┃   8h = cs
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_RW   (0～4G)     ┃  10h = ds, es, fs, ss
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_VIDEO                 ┃  1Bh = gs
	;              ┗━━━━━━━━━━━━━━━━━━┛
	;
	; 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
	; 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es.
	;
	;


	; 把 esp 从 LOADER 挪到 KERNEL
	mov	esp, StackTop	; 堆栈在 bss 段中

	mov	dword [disp_pos], 0

	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	lidt	[idt_ptr]

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:		; “这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.

	;jmp 0x40:0
	;ud2

;加载tr的代码
	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax

;跳转到kernel_main()函数中
	;sti
	jmp	kernel_main

	;hlt


; 中断和异常 -- 硬件中断
; ---------------------------------
; hwint_master的宏：先保存寄存器的值，再调用相应的函数，最后返回
%macro	hwint_master	1
	call	save
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断
	out	INT_M_CTLMASK, al	; /
	mov	al, EOI			; `. 置EOI位
	out	INT_M_CTL, al		; /
	sti	; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push	%1			; `.
	call	[irq_table + 4 * %1]	;  | 中断处理程序，调用的是irq_table[%1]
	pop	ecx			; /
	cli
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al	; /
	ret
%endmacro


;为了让时钟中断可以不停地发生而不是只发生一次，需要设置EOI
ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	hwint_master	0

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
	push	%1
	call	spurious_irq
	add	esp, 4
	hlt
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15



; 中断和异常 -- 异常
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt

; ====================================================================================
;                                   save
; ====================================================================================
save:
        pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /
        mov     dx, ss
        mov     ds, dx
        mov     es, dx

        mov     esi, esp                    ;esi = 进程表起始地址

        inc     dword [k_reenter]           ;k_reenter++;
        cmp     dword [k_reenter], 0        ;if(k_reenter ==0)
        jne     .1                          ;{
        mov     esp, StackTop               ;  mov esp, StackTop <--切换到内核栈
        push    restart                     ;  push restart
        jmp     [esi + RETADR - P_STACKBASE];  return;
.1:                                         ;} else { 已经在内核栈，不需要再切换
        push    restart_reenter             ;  push restart_reenter
        jmp     [esi + RETADR - P_STACKBASE];  return;
                                            ;}


; ====================================================================================
;                                 sys_call
; ====================================================================================
sys_call:
        call    save

        sti

		push 	ebx
        call    [sys_call_table + eax * 4] ; 调用的是sys_call_table [eax](调用sys_get_ticks)
		add		esp, 4
        mov     [esi + EAXREG - P_STACKBASE], eax ; 将函数sys_call_table[eax]的返回值放在进程表中eax的位置，以便进程P被恢复执行时eax中装的是正确的返回值

        cli

        ret


; ====================================================================================
;				    restart
; ====================================================================================
;实现从ring0到ring1的跳转
restart:
	mov	esp, [p_proc_ready] ;设置esp的值,p_proc_ready是一个指向进程表的指针，存放的是下一个要启动进程的进程表的地址
	lldt	[esp + P_LDT_SEL] ;设置ldtr的值,esp + P_LDT_SEL是s_proc的第一个成员ldt_sel
	lea	eax, [esp + P_STACKTOP]
	mov	dword [tss + TSS3_S_SP0], eax ;将s_proc中第一个结构体成员regs的末地址赋给TSS中ring0堆栈指针（esp）,在下一次中断发生时，esp将变成regs的末地址，进程ss、esp、eflags、cs、eip寄存器值依次被压栈，放在regs结构的最后面（堆栈从高地址向低地址生长）
restart_reenter:
	dec	dword [k_reenter] ;将k_reenter的值减1
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad
	add	esp, 4 ;esp的值加4（跳过retaddr成员）
	iretd

; 一个进程由“睡眠”状态变成“运行”状态，需要将esp指向进程表项的开始处，然后在执行lldt之后经历一系列pop指令恢复各个寄存器的值。
; 一切信息都包含在进程表中，若要恢复不同的进程，只需要将esp指向不同的进程表就可以了。

; p_proc_ready 其中的内容以下图所示顺序存放：(目的：使pop和popad指令执行后各寄存器的内容更新一遍)
	;        
	;              ┏━━━━━━┓   L
	;              ┃  gs  ┃   
	;              ┣━━━━━━┫   
	;              ┃  fs  ┃  /┃\
	;              ┣━━━━━━┫   ┃
	;              ┃  es  ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  ds  ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  edi ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  esi ┃   ┃
	;              ┣━━━━━━┫   ┃ 栈生长的方向
	;              ┃  ebp ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  esp ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  ebx ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  edx ┃   ┃
	;              ┣━━━━━━┫   ┃
	;              ┃  ecx ┃   ┃
	;              ┣━━━━━━┫
	;			   ┃  eax ┃   
    ;              ┗━━━━━━┛  H




; 中断处理程序
; 为了避免嵌套现象的发生，让中断处理程序知道自己是不是在嵌套执行，设置一个全局变量，当中断处理程序开始执行时它自加，结束时自减
; 在处理程序开头处检查这个全局变量，如果值不是0，则说明在一次中断未处理完之前又发生了一次中断，这时直接跳到最后，结束中断处理程序的执行
;extern k_reenter
;···
;[SECTION .data]
;···
; hwint00:            ;Interrupt routine for irq 0(the clock)
; 	sub esp,4		  ;跳过了进程表中的第一个成员
; 	pushad     ; `.
;	push ds    ;  |
;	push es    ;  | 保存原寄存器值
;	push fs    ;  |
;	push gs    ; /
;	mov dx,ss  
;	mov ds,dx  
;	mov es,dx  
;
;	inc byte [gs:0]                  ; 改变屏幕第0行，第0列的字符（以便看到效果）
;
;	mov al,EOI                       ; '. reenable
;	out INT_M_CTL,al                 ; / master 8295
;
;   inc dword [k_reenter]            ; k_reenter自加
;   cmp dword [k_reenter],0          ; 判断k_reenter是否为0
;   jne .1                           ; 重入时跳到.1，通常情况下顺序执行
;;; jne .re_enter
;
;	mov esp,StackTop                 ; 切到内核栈
;
;	push .restart
;	jmp .2
;
;.1:                                 ; 中断重入
;	push .restart_reenter
;
;.2:                                 ; 没有中断重入
;   sti                              ; CPU在响应中断的过程中会自动关闭中断，因此需要人为地打开中断，加入sti指令
;
;	push 0
;	call clock_handler
;	add esp,4
;
;	cli
;
;	ret                              ; 重入时跳到.restart_reenter_v2,通常情况下到.restart_v2
;
;;;.restart_v2:
;;; push 1
;;; call delay                       ; 为保证中断处理过程足够长，以至于在它完成之前就会有下一个中断产生，因此调用一个延迟函数
;;; add esp,4
;;;
;;;	mov esp,[p_proc_ready]           ; 离开内核栈
;;;	lldt [esp+P_LDT_SEL]
;;;
;;;	lea eax,[esp+P_STACKTOP]
;;;	mov dword [tss+TSS3_S_SP0],eax
;;;
;;;.restart_reenter_v2:                ; 如果[k_reenter ！= 0]，则跳转到这里
;;;.re_enter:                        ; 如果[k_reenter ！= 0]，则跳转到这里
;;; dec dword [k_reenter]
;;;	pop gs     ; `.   
;;;	pop fs     ;  |
;;;	pop es     ;  | 恢复原寄存器值
;;;	pop ds     ;  |
;;;	popad      ; /
;;;	add esp,4
;;;
;;;	iretd