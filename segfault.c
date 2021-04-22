#include<setjmp.h>
#include<signal.h>
#include<stdio.h>
#define samples 50
jmp_buf buf;
int test_memory_address;
int array[1]={1};

static void unblock_signal(int signum __attribute__((__unused__)))
{
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs,signum);
	sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

static void segfault_handler(int signum)
{
	(void) signum;
	unblock_signal(SIGSEGV);
	longjmp(buf,1);
}

static __inline__ void maccess(void *p) {
	  asm volatile("lfence;clflush (%0) \n" ::"c"(p));
}

int main(int argc, char** argv)
{
	if(signal(SIGSEGV, segfault_handler) == SIG_ERR)
	{
		printf("Failed");
		return -1;
	}
	int nFault = 0;
	for(int i=0; i < samples; i++)
	{
		test_memory_address += 4096;
		if(!setjmp(buf))
		{
			maccess(&array[test_memory_address]);
			continue;
		}
		nFault++;
	}
	printf("nfault is %d\n",nFault);
}
