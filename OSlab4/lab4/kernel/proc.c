
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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

/*======================================================================*
                              schedule
 *======================================================================*/
/* 进程调度 */
PUBLIC void schedule()
{
	PROCESS* p;
	//int	 greatest_ticks = 0;
	int	 current_tick = get_ticks();

	while (TRUE) {
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS) {
			p_proc_ready = proc_table;
		}
		if (p_proc_ready->isBlocked == FALSE && 
			p_proc_ready->wake_time <= current_tick) {
			break; // 寻找到进程
		}
	}

	/* for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	 	if (p->wake_tick > 0) {
	 		p->wake_tick--;
	 	}
	 }

	while (!greatest_ticks) {
	 	for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	 		if (p_proc_ready->wake_tick > current_tick || p->isBlocked == TRUE) {
	 			continue;
	 		}
	 		if (p->ticks > greatest_ticks) {
	 			greatest_ticks = p->ticks;
	 			p_proc_ready = p;
	 		}
	 	}

		//为进程表中的成员ticks重新赋值，以便让进程永远执行下去
	 	if (!greatest_ticks) {
	 		for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	 			if (p->ticks > 0) { // 说明被阻塞了
	 				continue;
	 			}
	 			p->ticks = p->priority;
	 		}
	 	}
	}*/
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC int sys_sleep(int milli_seconds) {
	p_proc_ready->wake_time = get_ticks() + (milli_seconds / (1000 / HZ));
	schedule(); // 进程调度
}

/*======================================================================*
                           sys_print
 *======================================================================*/
PUBLIC int sys_print(char *str)
{
	if (disp_pos >= 80 * 25 * 2) {
		memset(0xB8000, 0, 80 * 25 * 2);
		disp_pos = 0;
	}

	/* O打印绿色，X打印红色，Z打印蓝色 */
	if (*str == 'O') {
		disp_color_str(str, GREEN);
	} else if (*str == 'X') {
		disp_color_str(str, RED);
	} else if (*str == 'Z') {
		disp_color_str(str, BLUE);
	} else {
		disp_str(str);
	}
}

/* PV操作：实现进程间的同步与互斥 */

/*======================================================================*
                           sys_P
 *======================================================================*/
/*  
P(S)：
	① 将信号量S的值减1，即S=S-1；
	② 如果S>=0，则该进程继续执行；否则进程进入等待队列，置为等待状态。 
*/
PUBLIC int sys_P(void *semaphore)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)semaphore;
	s->value--;
	if (s->value < 0) {
		p_proc_ready->status = 0;
		p_proc_ready->isBlocked = TRUE;
		s->process_queue[s->tail] = p_proc_ready; // 将进程加入队列尾
		s->tail = (s->tail + 1) % NR_TASKS;
		schedule();
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_V
 *======================================================================*/
/*
V(S)：
	1.将信号量S的值加1，即S=S+1；
	2.如果S>0，则该进程继续执行；否则释放等待队列中第一个等待信号量的进程。
	(因为将信号量加1后仍然不大于0，则表示等待队列中有阻塞的进程)
*/
PUBLIC int sys_V(void *semaphore)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)semaphore;
	s->value++;
	if (s->value <= 0) {
		PROCESS *proc = s->process_queue[s->head]; // 释放信号量队列头部的进程
		proc->status = 0;
		proc->isBlocked = FALSE;
		s->head = (s->head + 1) % NR_TASKS;
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           READER
 *======================================================================*/
PUBLIC void READER(int time_slice)
{
	if(STRATEGY==0){
		//读者优先
		READER_reader_first(time_slice);
	}else if(STRATEGY==1){
		//写者优先
		READER_writer_first(time_slice);
	}else if(STRATEGY==2){
		//读写公平
		READER_fair(time_slice);
	}
}

/*======================================================================*
                           WRITER
 *======================================================================*/
PUBLIC void WRITER(int time_slice)
{
	if(STRATEGY==0){
		//读者优先
		WRITER_reader_first(time_slice);
	}else if(STRATEGY==1){
		//写者优先
		WRITER_writer_first(time_slice);
	}else if(STRATEGY==2){
		//读写公平
		WRITER_fair(time_slice);
	}
}

/* 读者优先 */
/*======================================================================*
                           READER_rf
 *======================================================================*/
void READER_reader_first(int time_slice)
{
	P(&mutex_readerNum);

	// 有读者的时候，限制写进程
	if (readerNum == 0){
		P(&writeBlock);
	}

	readerNum++;
	V(&mutex_readerNum);
	P(&readerLimit);

	p_proc_ready->status = 1; // 状态设置为正在读
	sleep(time_slice * TIME_SLICE);
	P(&mutex_readerNum);
	readerNum--;

	// 没有读者的时候，不限制写进程
	if (readerNum == 0){
		V(&writeBlock);
	}

	V(&mutex_readerNum);
	V(&readerLimit);
}

/*======================================================================*
                           WRITER_rf
 *======================================================================*/
void WRITER_reader_first(int time_slice)
{
	P(&writeBlock);

	p_proc_ready->status = 1; // 状态设置为正在写
	sleep(time_slice * TIME_SLICE);

	V(&writeBlock);
}

/* 写者优先 */
/*======================================================================*
                           READER_wf
 *======================================================================*/
void READER_writer_first(int time_slice)
{
	P(&readerLimit);
	P(&readBlock);
	P(&mutex_readerNum);

	// 有读者的时候，限制写进程
	if (readerNum == 0){
		P(&writeBlock);
	}

	readerNum++;
	V(&mutex_readerNum);
	V(&readBlock);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;

	// 无读者，可写
	if (readerNum == 0){
		V(&writeBlock);
	}

	V(&mutex_readerNum);
	V(&readerLimit);
}

/* 读写公平，防止进程饿死 */
/*======================================================================*
                           WRITER_wf
 *======================================================================*/
void WRITER_writer_first(int time_slice)
{
	P(&mutex_writerNum);

	// 有写者的时候，则禁止读
	if (writerNum == 0){
		P(&readBlock);
	}

	writerNum++;
	V(&mutex_writerNum);

	// 开始写
	P(&writeBlock);

	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	V(&writeBlock);
	// 完成写
	P(&mutex_writerNum);
	writerNum--;

	// 没有写者的时候，可读
	if (writerNum == 0){
		V(&readBlock);
	}

	V(&mutex_writerNum);
}


/* 读写公平，防止饿死 */
/*======================================================================*
                           READER_fair
 *======================================================================*/
void READER_fair(int time_slice)
{
	// 开始读
	P(&mutex_fair);

	P(&readerLimit);
	P(&mutex_readerNum);
	if (readerNum == 0){
		P(&writeBlock);
	}
	V(&mutex_fair);

	readerNum++;
	V(&mutex_readerNum);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0){
		V(&writeBlock);
	}
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_fair
 *======================================================================*/
void WRITER_fair(int time_slice)
{

	P(&mutex_fair);
	P(&writeBlock);
	V(&mutex_fair);

	// 开始写
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);
	
	// 完成写
	V(&writeBlock);
}
