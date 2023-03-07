
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
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
                           clock_handler
 *======================================================================*/
/* 时钟中断处理程序 */
PUBLIC void clock_handler(int irq)
{
	/*
	每一次让p_proc_ready指向进程表中的下一个表项，如果切换前已经到达进程表结尾则回到第一个表项
	当发生中断重入时，本函数会直接返回，不做任何操作
	disp_str("#");
	ticks++;

	//加入打印字符"!"的代码，是为了发生中断重入的时候可以直观地看到
	if(k_reenter != 0) {
		disp_str("!");
		return;
	}

	p_proc_ready++;
	if(p_proc_ready >= proc_table + NR__TASKS)
		p_proc_ready = proc_table;
	*/

	ticks++;
	p_proc_ready->ticks--;

	if (k_reenter != 0) {
		return;
	}

	/* 在一个进程的ticks还没有变成0之前，其他进程就不会有机会获得执行 */
	if (p_proc_ready->ticks > 0) {
		return;
	}

	schedule();

}

/*======================================================================*
                              milli_delay
 *======================================================================*/
/* 精确到10ms的延迟函数 */
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks(); //得到当前ticks值

		/*  
		开始循环：
			每次循环的时候看已经过去了多少个ticks(假设是Δt个)
			因为ticks之间的间隔时间是(1000/Hz)ms,所以Δt个ticks相当于(Δt*1000/Hz)ms
			循环会在这个毫秒数大于要求的毫秒数时退出
		*/
        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

