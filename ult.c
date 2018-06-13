#include "ult.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

/* thread control block */
typedef struct tcb_s
{
	int id;				/* the id of the thread */
	status_e status;	/* the status: running/finished */
	int returnValue;	/* the return value */
	int fd;				/* file descriptor of current read */
	ucontext_t context;	/* the ucontext structure */
} tcb_t;

typedef struct threads_s
{
	tcb_t* threads;		/* the threads */
	size_t size;		/* how many threads there are */
	size_t malloced;	/* how many space we have */
	size_t currentThreadIndex;	/* the thread currently in execution */
} threads_t;

static threads_t threads; 		/* the scheduler */

static int hasInput(int fd)
{
	/* set of fds */
	fd_set rfds;
    struct timeval tv;

    /* put in the requested fd */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    /* timout: 0.0 seconds */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /* check if data is available for reading */
    int retval = select(fd + 1, &rfds, NULL, NULL, &tv);

    if (retval == -1) 
    {
    	perror("select() failed");
    }
    else if (FD_ISSET(fd, &rfds)) 
    {
    	return 1;
    }
    else
    {
    	return 0;
    }
	
	return 1;
}

void ult_init(ult_f f)
{
	/* initalize the structure to some defaults */
	threads.size = 0;
	threads.malloced = 4;
	threads.threads = (tcb_t*)malloc(sizeof(tcb_t) * threads.malloced);
	threads.currentThreadIndex = 0;

	int initID = ult_spawn(f); /* start the init thread */
	setcontext(&(threads.threads[initID].context));
}

int ult_spawn(ult_f f)
{	
	/* get a pointer to the next available thread */
	tcb_t *newThread = &(threads.threads[threads.size]);

	/* mark it as running */
	newThread->status = STATUS_RUNNING;

	/* initialize the context by cloning ours */
	getcontext(&(newThread->context));

	/* create the new stack */
	newThread->context.uc_link = 0;
	newThread->context.uc_stack.ss_flags = 0;
	newThread->context.uc_stack.ss_size = STACK_SIZE;
	newThread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	newThread->id = threads.size;
	newThread->fd = -1;

	/* check if the stack creation worked */
	if (newThread->context.uc_stack.ss_sp == NULL)
	{
		fprintf(stderr, "%s\n", strerror(errno));
		return -1;
	}
	
	/* modify the context */
	makecontext(&(newThread->context), f, 0);

	threads.size++;

	/* increase array size on reaching off limit */
	if(threads.size == threads.malloced)
	{
		threads.malloced *= 2;
		tcb_t *temp = (tcb_t*)realloc(threads.threads, threads.malloced);

		if(temp == NULL)
		{
			/* report an error but dont exit all the other threads */
			fprintf(stderr, "%s\n", strerror(errno));
			return -1;
		}

		threads.threads = temp;
	}

	return (threads.size - 1);		
}

ssize_t yieldFDThreads()
{
	size_t yieldingThread = threads.currentThreadIndex;

	do
	{
		/* work through the list and find the next runable thread */
		threads.currentThreadIndex = (threads.currentThreadIndex + 1) % threads.size;
	}
	while(((threads.threads[threads.currentThreadIndex].status == STATUS_FINISHED)
		|| (threads.threads[threads.currentThreadIndex].status == STATUS_JOINED)) 
		&& (threads.threads[threads.currentThreadIndex].fd < 0 
		|| !hasInput(threads.threads[threads.currentThreadIndex].fd)));

	if(yieldingThread == threads.currentThreadIndex) 
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void ult_yield()
{
	/* save the current thread id */
	size_t yieldingThread = threads.currentThreadIndex;

	ssize_t res = yieldFDThreads();

	if(res < 0)
	{
		do
		{
			/* work through the list and find the next runable thread */
			threads.currentThreadIndex = (threads.currentThreadIndex + 1) % threads.size;
		}
		while(((threads.threads[threads.currentThreadIndex].status == STATUS_FINISHED)
			|| (threads.threads[threads.currentThreadIndex].status == STATUS_JOINED))
			&& threads.threads[threads.currentThreadIndex].fd > -1);

		/* return if the next runable thread already runs */
		if(yieldingThread == threads.currentThreadIndex) 
		{
			usleep(100000);
			ult_yield();
			return;
		}
	}

	/* else swap to that thread */
	swapcontext(&(threads.threads[yieldingThread].context), 
		&(threads.threads[threads.currentThreadIndex].context));
}

void ult_exit(int status)
{
	/* save the return value */
	threads.threads[threads.currentThreadIndex].returnValue = status;
	/* mark the thread as finished */
	threads.threads[threads.currentThreadIndex].status = STATUS_FINISHED;
}

int ult_join(int tid, int* status)
{
	/* thread does not exist */
	if(tid < 0 || tid >= threads.size || threads.threads[tid].status == STATUS_JOINED) 
	{
		return -1;
	}

	/* wait until the thread with the given id is finished */
	while(threads.threads[tid].status == STATUS_RUNNING) 
	{
		ult_yield();
	}

	/* we cant join a thread multiple times, so we mark it as joined */
	threads.threads[tid].status = STATUS_JOINED;
	
	/* copy the return value */
	*status = threads.threads[tid].returnValue;

	/* and free the memory */
	free(threads.threads[tid].context.uc_stack.ss_sp);

	return 0;
}

int ult_gettid()
{
	return threads.threads[threads.currentThreadIndex].id;
}

ssize_t ult_read(int fd, void* buf, size_t size)
{
	threads.threads[threads.currentThreadIndex].fd = fd;
	/* check if data is available */
	if(hasInput(fd))
	{
		/* read the data if yes */
		return read(fd, buf, size);
	}
	else
	{
		/* go to the scheduler if no */
		ult_yield();
	}

	return 0;
}
