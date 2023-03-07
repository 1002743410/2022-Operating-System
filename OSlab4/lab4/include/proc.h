
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* 进程表结构体 */
typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;		/* ┓						│			*/
	u32	fs;		/* ┃						│			*/
	u32	es;		/* ┃						│			*/
	u32	ds;		/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/		
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	eip;		/*  ┓						│			*/
	u32	cs;		/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;		/*  ┛						┷High			*/
}STACK_FRAME;


typedef struct s_proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

        int ticks;                 /* remained ticks,ticks是递减的，从某个初值到0 */
        int priority;              /* 恒定不变，当所有的进程ticks都变为0之后，再把各自的ticks赋值为priority，然后继续执行 */

	int status;		/* 进程状态(0：等待读或写,1:正在读或写,2:休息) */
	int wake_time;	/* 进程被唤醒时间 */
	int isBlocked; 	/* 进程被阻塞 */

	u32 pid;                   /* process id passed in from MM */
	char p_name[16];           /* name of the process */
}PROCESS;


/* 进程结构体 */
typedef struct s_task {
	task_f	initial_eip;//进程体
	int	stacksize;//堆栈
	char	name[32];//进程的名字
}TASK;


/* Number of tasks */
#define NR_TASKS	6  //修改NR_TASKS为6，增加A、B、C、D、E、F六个进程

/* stacks of tasks */
/* 增加进程A、B、C、D、E、F的STACK_SIZE的定义 */
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000
#define STACK_SIZE_TESTD	0x8000
#define STACK_SIZE_TESTE	0x8000
#define STACK_SIZE_TESTF	0x8000

#define STACK_SIZE_TOTAL	(STACK_SIZE_TESTA + \
							 STACK_SIZE_TESTB + \
							 STACK_SIZE_TESTC + \
							 STACK_SIZE_TESTD + \
							 STACK_SIZE_TESTE + \
							 STACK_SIZE_TESTF)


/* 信号量结构体 */
typedef struct s_semaphore
{
	int value; /* 资源量 */
	int head; /* 头部 */
	int tail; /* 尾部 */
	PROCESS * process_queue[NR_TASKS]; /* 等待信号量的进程队列 */
} SEMAPHORE;