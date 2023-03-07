
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS]; //声明一个进程表

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

/* 增加进程A、B、C、D、E、F的定义 */
PUBLIC	TASK	task_table[NR_TASKS] = {{TestA, STACK_SIZE_TESTA, "TestA"},
					                    {TestB, STACK_SIZE_TESTB, "TestB"},
					                    {TestC, STACK_SIZE_TESTC, "TestC"},
                                        {TestD, STACK_SIZE_TESTD, "TestD"},
					                    {TestE, STACK_SIZE_TESTE, "TestE"},
					                    {TestF, STACK_SIZE_TESTF, "TestF"}};

PUBLIC	irq_handler		irq_table[NR_IRQ];

/* 添加系统调用 */
PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                       sys_sleep,
                                                       sys_print,
                                                       sys_P,
                                                       sys_V};

PUBLIC SEMAPHORE readerLimit = {MAX_READERS, 0, 0};
PUBLIC SEMAPHORE writeBlock = {1, 0, 0};
PUBLIC SEMAPHORE readBlock = {1, 0, 0};
PUBLIC SEMAPHORE mutex_readerNum = {1, 0, 0};
PUBLIC SEMAPHORE mutex_writerNum = {1, 0, 0};
PUBLIC SEMAPHORE mutex_fair = {1, 0, 0};