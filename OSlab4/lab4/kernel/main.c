
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/* 
Orange's的运转过程：
	1.控制权交给引导扇区 ---------------------------------------
	2.加载loader                                 BOOT SECTOR
	3.跳到loader-----------------------------------------------
	4.加载kernel                                 LOADER
	5.跳到kernel-----------------------------------------------
	6.切换至kernel的GDT                          KERNEL
  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
  ┃	7.初始化8259A                                             ┃
  ┃	8.初始化IDT                                               ┃   init_prot()
  ┃	9.初始化GDT中的TSS和LDT两个描述符,以及初始化TSS              ┃
  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
  ┃	10.初始化进程表                                            ┃
  ┃	11.指定时钟中断处理程序                                     ┃ kernel_main()
  ┃	12.让8259A可以接受时钟中断                                  ┃
  ┃	13.restart()                                              ┃
  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
	14.进程开始运转（进程A、进程B、进程C、进程D、进程E、进程F）<---时钟中断
*/

/*
进程启动过程：
	1.准备好进程体
	2.初始化GDT中的TSS和LDT两个描述符，以及初始化TSS(在init_prot()中完成)
	3.准备进程表(在kernel.main()中完成)
	4.完成跳转，实现ring0->ring1(kernel.asm之restart)
*/

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	/* 
	1.由于堆栈是从高地址往低地址生长，所以在给每一个进程分配栈空间的时候也是从高地址往低地址进行
	2.为每一个进程都在GDT中分配一个描述符用来对应进程的LDT
	 */

	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table; //声明进程表
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST; //LDTSelector被赋值为SELECTOR_LDT_FIRST
	int i;

	/*初始化进程表
	主要包括三部分：寄存器、LDTSelector、LDT
	LDT里有两个描述符，分别被初始化为内核代码段和内核数据段
	*/
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			            // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		//cs指向LDT中的第一个描述符
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		//ds、es、fs、ss指向LDT中的第二个描述符
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		//gs指向显存
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		//eip指向进程任务，表明进程将从任务的入口地址开始运行
		p_proc->regs.eip = (u32)p_task->initial_eip;
		//esp指向单独的栈，栈的大小为p_task_stack
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* 0x1202设置了IF=1, IOPL=1，使进程可以使用I/O指令，并且中断会在iretd执行时被打开 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	/* 优先级调度 */
	for (int i = 0; i < NR_TASKS; i++) {
		proc_table[i].ticks = 1;
		proc_table[i].priority = 1;
		proc_table[i].wake_time = 0;
		proc_table[i].status = 2;
		proc_table[i].isBlocked = 0;
	}

	k_reenter = 0; // 在进程第一次运行之前执行了dec dword [k_reenter]，因此修改k_reenter的值为0
	ticks = 0;

	p_proc_ready	= proc_table;

		/* 设置计数值 */
        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	cleanScreen(); // 清屏

	restart(); //调用kernel.asm中函数，是进程调度的一部分（操作系统启动第一个进程时的入口）

	while(1){} //用一个死循环作为进程的结束
}

/* 添加清屏函数 */
PUBLIC void cleanScreen() {
	disp_pos = 0;
	for (int i = 0; i < 80 * 25; i++) {
		disp_str(" ");
	}
	disp_pos = 0;//将显存指针指向第一个位置

	// 初始化读者数量与写者数量
	readerNum = 0;
	writerNum = 0;
}

/*
PUBLIC void delay(int time){
	int i,j,k;
	for(k=0;k<time;k++){
		for(i=0;i<10;i++){
			for(j=0;j<10000;j++){}
		}
	}
}

进程执行体
每循环一次打印一个字符和一个数字，并且稍等片刻
void TestA(){
	int i=0;
	while(TRUE){
		disp_str("A");
		dis_int(get_ticks()); //打印系统调用
		disp_str(".");
		milli_delay(1000);
	}
}

void TestB(){
	int i=0x1000;//分辨A、B两个进程
	while(TRUE){
		disp_str("B");
		dis_int(get_ticks()); //打印系统调用
		disp_str(".");
		milli_delay(1000);
	}
}
*/


/* 
添加一个任务步骤：
	1.在main.c中添加一个进程体；
	2.在global.c的task_table中添加一项进程；
	3.在proc.h中修改NR_TASKS的值；
	4.在proc.h中添加进程堆栈大小STACK_SIZE的声明；
	5.在proc.h中修改STACK_SIZE_TOTAL；
	6.在proto.h中添加进程体的函数声明。
*/

/* 
添加一个系统调用步骤：
	1.在const.h中，NR_SYS_CALL加一；
	2.在global.c中的sys_call_table[]增加一个成员，假设是sys_foo；
	3.sys_foo的函数体(因具体情况而异)；
	4.在proto.h中添加sys_foo的函数声明；
	5.在proto.h中添加foo的函数声明；
	6.在syscall.asm中添加_NR_foo的定义；
	7.在syscall.asm中添加global foo；
	8.在kernel.asm中，如果参数个数与以前的系统调用比有所增加，则需要修改sys_call。
*/

/* 普通进程A */
void TestA() {
	milli_delay(200);
	int n = 0;
	while (TRUE) {
		// 控制输出20次
		if (n++ < 20) {
			// 输出序号
			if(n < 10) {
				char tmp[4] = {n + '0', ' ', ' ', '\0'};
				print(tmp);
			} else {
				char tmp[4] = {(n / 10) + '0', (n % 10) + '0', ' ', '\0'};
				print(tmp);
			}

			// 输出读者写者进程的状态
			for (int i = 1; i < NR_TASKS; i++) {
				int status = proc_table[i].status;
				if (status == 0) {
					print("X "); // 等待读/写
				} else if (status == 1) {
					print("O "); // 正在读/写
				} else if (status == 2) {
					print("Z "); // 休息
				}
			}
			print("\n");
			milli_delay(TIME_SLICE);
		}
	}
}

/* 读者进程B */
void TestB() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0; // 等待读
		READER(2); // B阅读消耗2个时间片 
		p_proc_ready->status = 2; // 休息
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE*1); // 读完后休息1个时间片
	}
}

/* 读者进程C */
void TestC() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0; // 等待读
		READER(3); // C阅读消耗3个时间片 
		p_proc_ready->status = 2; // 休息
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE*1); // 读完后休息1个时间片
	}
}

/* 读者进程D */
void TestD() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0; // 等待读
		READER(3); // D阅读消耗3个时间片 
		p_proc_ready->status = 2; // 休息
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE*1); // 读完后休息1个时间片
	}
}

/* 写者进程E */
void TestE() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0; // 等待写
		WRITER(3); // E写消耗3个时间片
		p_proc_ready->status = 2; // 休息
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE*1); // 写完后休息1个时间片
	}
}

/* 写者进程F */
void TestF() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0; // 等待写
		WRITER(4); // F写消耗4个时间片 
		p_proc_ready->status = 2; // 休息
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE); // 写完后休息1个时间片
	}
}
