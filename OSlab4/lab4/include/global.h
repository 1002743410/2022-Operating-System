
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		ticks;

EXTERN	int		disp_pos;
EXTERN	u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;

EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready; //指向进程表结构的指针

extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK            task_table[];//进程
extern	irq_handler	irq_table[];

EXTERN int readerNum; //记录读者数量
EXTERN int writerNum; //记录写者数量

EXTERN SEMAPHORE readerLimit;     // 同时读同一本书的读者数量
EXTERN SEMAPHORE writeBlock;      // 限制写进程
EXTERN SEMAPHORE readBlock;       // 限制读进程，保证写优先
EXTERN SEMAPHORE mutex_readerNum; // 保护 readerNum 的变化
EXTERN SEMAPHORE mutex_writerNum; // 保护 writerNum 的变化
EXTERN SEMAPHORE mutex_fair;      // 实现读写公平