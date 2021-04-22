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
#include <sys/mman.h>
#include <sys/stat.h>
#define threshold 140
#define space 4096
#define no_of_lines 256
int *userArray;
void *target;
uint32_t *timing;
double reliablity;
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

void clflush(volatile void *Tx)
{
	asm volatile("lfence;clflush (%0) \n" ::"c"(Tx));
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
	asm volatile("lfence");
	t2 = timer_stop();
	return t2 - t1;
}

int main()
{
	userArray = malloc(sizeof(int) * no_of_lines * space);
	memset(userArray, 1, sizeof(int) * no_of_lines * space);
	int data;
	time_t t;
	srand((unsigned)time(&t));
	uint32_t timing;
	int recieved_data;
	int count = 0;

	for (int j = 0; j < 100000; j++)
	{
		data = rand() % no_of_lines;
		asm volatile("mfence");

		for (int i = 0; i < no_of_lines; i++)
		{
			clflush(&userArray[i * space]);
		}
		maccess(&userArray[data * space]);
		for (int i = 0; i < no_of_lines; i++)
		{
			timing = reload(&userArray[i * space]);
			if (timing < threshold)
			{
				recieved_data = i;
				break;
			}
		}
		if (recieved_data == data)
			count++;
	}
	printf("Reliablity : %f\n", (float)(count/1000));
	return 0;
}
