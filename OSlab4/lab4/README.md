# 2022年秋季操作系统实验（四）

[TOC]

## 1.添加进程

### 1.1 步骤

> - 在main.c中添加一个进程体；
> - 在global.c的task_table中添加一项进程；
> - 在proc.h中修改NR_TASKS的值；
> - 在proc.h中添加进程堆栈大小STACK_SIZE的声明；
> - 在proc.h中修改STACK_SIZE_TOTAL；
> - 在proto.h中添加进程体的函数声明。

### 1.2 实现

- 在kernel/main.c中添加A、B、C、D、E、F的进程执行体

  ```c++
  void TestA() {}
  void TestB() {}
  void TestC() {}
  void TestD() {}
  void TestE() {}
  void TestF() {}
  ```

- 在kernel/global.c的task_table中添加进程A、B、C、D、E、F的定义

  ```c++
  PUBLIC	TASK	task_table[NR_TASKS] = {{TestA, STACK_SIZE_TESTA, "TestA"},
  					                    {TestB, STACK_SIZE_TESTB, "TestB"},
  					                    {TestC, STACK_SIZE_TESTC, "TestC"},
                                          {TestD, STACK_SIZE_TESTD, "TestD"},
  					                    {TestE, STACK_SIZE_TESTE, "TestE"},
  					                    {TestF, STACK_SIZE_TESTF, "TestF"}};
  ```

- 在include/proc.h 中修改 NR_TASKS 的值为6，添加进程堆栈大小STACK_SIZE的声明，并修改 STACK_SIZE_TOTAL

  ```c++
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
  ```

- 在include/proto.h中添加进程体的函数声明

  ```c++
  /* main.c */
  /* 增加对进程A、B、C、D、E、F的函数声明 */
  void TestA();
  void TestB();
  void TestC();
  void TestD();
  void TestE();
  void TestF();
  ```

## 2.添加系统调用

### 2.1 步骤

> 1.在const.h中，NR_SYS_CALL加一；
>
>   2.在global.c中的sys_call_table[]增加一个成员，假设是sys_foo；
>
>   3.sys_foo的函数体(因具体情况而异)；
>
>   4.在proto.h中添加sys_foo的函数声明；
>
>   5.在proto.h中添加foo的函数声明；
>
>   6.在syscall.asm中添加_NR_foo的定义；
>
>   7.在syscall.asm中添加global foo；
>
>   8.在kernel.asm中，如果参数个数与以前的系统调用比有所增加，则需要修改sys_call。

### 2.2 实现

- 在include/const.h中修改 NR_SYS_CALL 的值为5

  ```c++
  /* system call */
  #define NR_SYS_CALL     5
  ```

- 在kernel/global.c中，添加sys_call_table[]的系统调用成员

  ```c++
  PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                         sys_sleep,
                                                         sys_print,
                                                         sys_P,
                                                         sys_V};
  ```

- 在kernel/syscall.asm中添加sleep、print、P、V函数体的实现

  ```nasm
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
  ```

- 在include/proto.h中添加sleep、print、P、V系统调用函数的声明

  ```c++
  PUBLIC  int     sys_sleep(int milli_seconds);
  PUBLIC  int     sys_print(char *str);
  PUBLIC  int     sys_P(void *semaphore);
  PUBLIC  int     sys_V(void *semaphore);
  ```

- 在include/proto.h中添加sleep、print、P、V函数的声明

  ```c++
  PUBLIC  int     sleep(int milli_seconds);
  PUBLIC  int     print(char *str);
  PUBLIC  int     P(void *semaphore);
  PUBLIC  int     V(void *semaphore);
  ```

- 在kernel/syscall.asm中添加_NR_sleep\print\P\V的定义

  ```nasm
  _NR_sleep	    	equ 1
  _NR_print  			equ 2
  _NR_P      			equ 3
  _NR_V       		equ 4
  ```

- 在kernel/syscall.asm中添加global sleep\print\P\V

  ```nasm
  global 	sleep
  global 	print
  global 	P
  global 	V
  ```

- 如果参数个数与以前的系统比有所增加，则需要修改kernel/kernel.asm中的sys_call

  ```nasm
  sys_call:
      call	save
      sti
      push	ebx
      call	[sys_call_table + eax * 4]
      add		esp, 4
      mov 	[esi + EAXREG - P_STACKBASE], eax
      cli
      ret
  ```

### 2.3 系统调用—sleep

#### 2.3.1 功能

> 接受⼀个 `int` 型参数 `milli_seconds` ，调用此系统调用的进程会在数 `milli_seconds` 毫秒内不被分配时间片

#### 2.3.2 实现

- 在include/proc.h中的结构体s_proc添加进程状态、被唤醒时间和是否被阻塞状态

  ```c++
  int status;		/* 进程状态(0：等待读或写,1:正在读或写,2:休息) */
  int wake_time;	/* 进程被唤醒时间 */
  int isBlocked; 	/* 进程被阻塞 */
  ```

- 在kernel/proc.c 中添加sys_sleep()系统调用函数体的实现（假设过去Δt个ticks，因为ticks之间的间隔是(1000 / Hz)ms，所以Δt个ticks相当于(Δt*1000/Hz)ms，故用当前ticks值加上(传入的毫秒数/1000Hz)为进程唤醒的时间）

  ```c++
  PUBLIC int sys_sleep(int milli_seconds) {
  	p_proc_ready->wake_time = get_ticks() + (milli_seconds / (1000 / HZ));
  	schedule(); //进程调度
  }
  ```

- 在kernel/main.c中为系统调用进行初始化

  ```c++
  PUBLIC int kernel_main()
  {
  	...
  	/* 优先级调度 */
  	for (int i = 0; i < NR_TASKS; i++) {
  		proc_table[i].ticks = 1;
  		proc_table[i].priority = 1;
  		proc_table[i].wake_time = 0;
  		proc_table[i].status = 2;
  		proc_table[i].isBlocked = 0;
  	}
  ```

### 2.4 系统调用—print

#### 2.4.1 功能

> 接受 `char*` 型参数 `str`，打印字符串

#### 2.4.2 实现

- 在include/const.h中添加颜色常量和宏函数的定义

  ```c++
  #define BLACK   0x0     /* 0000 */
  #define WHITE   0x7     /* 0111 */
  #define RED     0x4     /* 0100 */
  #define GREEN   0x2     /* 0010 */
  #define BLUE    0x1     /* 0001 */
  #define FLASH   0x80    /* 1000 0000 */
  #define BRIGHT  0x08    /* 0000 1000 */
  #define MAKE_COLOR(x,y) (x | y) /* MAKE_COLOR(Background,Foreground) */
  ```

- 在kernel/proc.c中添加sys_print()系统调用函数体的实现

  ```c
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
  ```

### 2.5 系统调用—PV操作

#### 2.5.1 功能

> 添加两个系统调用执行信号量PV操作，在此基础上模拟读者写者问题。普通进程A每个时间片输出每个读者写者的状态，格式为： `[序号] [B] [C] [D] [E] [F]` ，如 `1 O O O X X` ，每个状态用对应的符号加上对应的颜色表示。为了方便检查，只输出20次(序号从1 ~ 20)

#### 2.5.2 实现

- 在include/proc.h中添加s_semaphore结构体的定义

  ```c++
  /* 信号量结构体 */
  typedef struct s_semaphore
  {
      int value; /* 资源量 */
  	int head; /* 头部 */
  	int tail; /* 尾部 */
  	PROCESS * process_queue[NR_TASKS]; /* 等待信号量的进程队列 */
  } SEMAPHORE;
  ```

- 在kernel/proc.c中添加执行信号量P、V操作的系统调用函数的实现

  ```c++
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
  		PROCESS *proc = s->process_queue[s->head]; // 释放等待队列头部的进程
  		proc->status = 0;
  		proc->isBlocked = FALSE;
  		s->head = (s->head + 1) % NR_TASKS;
  	}
  	enable_irq(CLOCK_IRQ);
  }
  ```

## 3.进程调度

### 3.1 功能

> 同时读的数量𝑛要求𝑛 = 1, 2, 3均要实现，要求能够现场修改；读（写）完后休息的时间𝑡(𝑡 ≥ 0)可自定，每个进程休息时间可不同，要求能够现场修改。

### 3.2 实现

- 在include/const.h中定义可以同时读的数量、一个时间片长度和调度策略

  ```c++
  #define MAX_READERS     3   /* 可以同时读的数量 */
  #define TIME_SLICE      1000 /* 一个时间片长度 */
  #define STRATEGY        0   /* 调度策略(读优先:0,写优先:1,读写公平:2) */
  ```
  
- 在include/global.h中添加记录读者与写者数量变量的宏定义

  ```c++
  EXTERN int readerNum; //记录读者数量
  EXTERN int writerNum; //记录写者数量
  EXTERN SEMAPHORE mutex_readerNum; // 保护 readerNum 的变化
  EXTERN SEMAPHORE mutex_writerNum; // 保护 writerNum 的变化
  ```
  
- 在include/global.h中添加各信号量的定义

  ```c++
  EXTERN SEMAPHORE readerLimit;     // 同时读同一本书的读者数量
  EXTERN SEMAPHORE writeBlock;      // 限制写进程
  EXTERN SEMAPHORE readBlock;       // 限制读进程
  EXTERN SEMAPHORE mutex_fair;      // 实现读写公平
  ```
  
- 在kernel/global.c中初始化各信号量

  ```c++
  PUBLIC SEMAPHORE mutex_readerNum = {1, 0, 0};
  PUBLIC SEMAPHORE mutex_writerNum = {1, 0, 0};
  PUBLIC SEMAPHORE readerLimit = {MAX_READERS, 0, 0};
  PUBLIC SEMAPHORE writeBlock = {1, 0, 0};
  PUBLIC SEMAPHORE readBlock = {1, 0, 0};
  PUBLIC SEMAPHORE mutex_fair = {1, 0, 0};
  ```

## 4.读者写者问题

### 4.1 模拟读者写者问题

 #### 4.1.1 功能

> 模拟读者写者问题。

#### 4.1.2 实现

- 在include/proto.h中添加读者与写者接口函数的声明

  ```c++
  /* 读写接口函数 */
  PUBLIC void WRITER(int time_slice);
  PUBLIC void READER(int time_slice);
  ```

- 在kernel/proc.c中添加读者写者的PV操作（根据调度策略STRATEGY的值不同选择具体的读者写者策略）

  ```c++
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
  ```

- 在kernel/main.c中完成A、B、C、D、E、F六个进程体的具体实现

  - 普通进程A

    ```c++
    /* 普通进程A */
    void TestA() {
    	milli_delay(200);
    	int n = 0;
    	while (TRUE) {
    		// 控制输出20次
    		if (n++ < 20) {
    			// 输出序号(dui'qi输出)
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
    ```

  - 读者进程B、读者进程C、读者进程C

    ```c++
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
    ```

  - 写者进程E、写者进程F

    ```c++
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
    ```

### 4.2 读者优先策略

#### 4.2.1 功能

> 请实现读者优先策略，要求能够现场修改。
>
> * 如果有读者在读，写者就得一直等待；
> * 如果写者进程在读，读者进程到来后，还没写的进程必须让读者先读

#### 4.2.2 实现

- 在include/proto.h中添加读者与写者策略函数的声明

  ```c++
  /* 读者优先 */
  PUBLIC void WRITER_reader_first(int time_slice);
  PUBLIC void READER_reader_first(int time_slice);
  ```

- 在kernel/proc.c中添加读者优先策略

  ```c++
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
  
  void WRITER_reader_first(int time_slice)
  {
  	P(&writeBlock);
  
  	p_proc_ready->status = 1; // 状态设置为正在写
  	sleep(time_slice * TIME_SLICE);
  
  	V(&writeBlock);
  }
  ```

#### 4.2.3 运行截图

##### 4.2.3.1 读并发量=1(出现写者饿死)

![n=1](https://box.nju.edu.cn/f/5480c5689f1c4bf4beff/?dl=1)

##### 4.2.3.2 读并发量=2(出现写者饿死)

![n=2](https://box.nju.edu.cn/f/5632b0685a6048e7b33b/?dl=1)

##### 4.2.3.3 读并发量=3(没有出现写者饿死))

![n=3](https://box.nju.edu.cn/f/d1949af154c943e1a564/?dl=1)

### 4.3 写者优先策略

#### 4.3.1 功能

> 请实现写者优先策略，要求能够现场修改。
>
> * 已有进程运行时，有写者和读者进入，即使读者晚于写者到来，也优先进行写操作
>
>   例如，对于到达顺序【R1】【W1】【R2】【W2】【R3】而言，运行顺序为
>
>   【R1】【W1】【W2】【R2】【R3】
>
> * 写者进程到来后，已经进入队列的读进程可以先读，还没到来的都必须等写完才可以读

#### 4.3.2 实现

- 在include/proto.h中添加写者策略函数的声明

  ```c++
  /* 写者优先 */
  PUBLIC void WRITER_writer_first(int time_slice);
  PUBLIC void READER_writer_first(int time_slice);
  ```

- 在kernel/proc.c中添加写者优先策略

  ```c++
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
  ```

#### 4.3.3 运行截图

##### 4.3.3.1 读并发量=1(出现读者饿死)

![n=1](https://box.nju.edu.cn/f/436b74eee80a46bdaa06/?dl=1)

##### 4.3.3.2 读并发量=2(出现读者饿死)

![n=2](https://box.nju.edu.cn/f/ffd9d465be434349a728/?dl=1)

##### 4.3.3.3 读并发量=3(没有出现读者饿死)

![n=3](https://box.nju.edu.cn/f/cb978efa84a446e2a1c7/?dl=1)

### 4.4 解决饿死问题—公平读写

#### 4.4.1 功能

> 请想办法解决此问题中部分情况下的进程饿死问题(不能通过调整读写后的休息时长来解决，即便 t = 0 时也要想办法解决)

#### 4.4.2 实现

- 在kernel/proc.c中添加读写公平的函数实现

  ```c++
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
  ```

#### 4.4.3 运行截图

##### 4.4.3.1 读并发量=1(没有出现进程饿死)

![n=1](https://box.nju.edu.cn/f/e5e4b6cd45184a488495/?dl=1)

##### 4.4.3.2 读并发量=2(没有出现进程饿死)

![n=2](https://box.nju.edu.cn/f/b55b8adee039465a8bad/?dl=1)

##### 4.4.3.3 读并发量=3(没有出现进程饿死)

![n=3](https://box.nju.edu.cn/f/6ed32d91756f4ad8b116/?dl=1)

## 5.补充

### 5.1 补充函数

- 在kernel/main.c中添加cleanScreen()函数（用于清屏和对变量的初始化）

  ```c++
  PUBLIC int kernel_main()
  {	...
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
  ```

- 修改kernel/proc.c中的schedule() 函数

  ```c++
  PUBLIC void schedule()
  {
  	PROCESS* p;
  	int	 current_tick = get_ticks();//当前调用
  
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
  }
  ```

### 5.2 运行步骤

1. 修改`Makefile`中buildimg文件路径；
2. 修改`const.h`中MAX_READERS，即选择读者并发量；
3. 修改`const.h`中STRATEGY，即选择具体的读写策略；
