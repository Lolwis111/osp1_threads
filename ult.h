
#ifndef ULT_H_INCLUDED
#define ULT_H_INCLUDED

#include <sys/types.h>

/* allow each Thread 8 KByte of Stack */
#define STACK_SIZE (8 * 1024)

typedef enum {
	STATUS_RUNNING,
	STATUS_FINISHED,
	STATUS_JOINED
} status_e;

/**@brief Type of function that can be scheduled.
 */
typedef void (*ult_f)();

/**@brief Returns the thread id of the calling thread.
 */
extern int ult_gettid();

/**@brief Initializes the thread library and executes the first thread.
 */
extern void ult_init(ult_f f);

/**@brief Spawns a new thread, returning its ID.
 */
extern int ult_spawn(ult_f f);

/**@brief Yields to the scheduler to switch to another thread.
 */
extern void ult_yield();

/**@brief Exits the current thread.
 */
extern void ult_exit(int status);

/**@brief Makes the current thread wait for the termination of another thread.
 * @return 0 for success, -1 if the thread doesn't exist
 */
extern int ult_join(int tid, int* status);

/**@brief Reads from an open file descriptor and yields to the scheduler if no
 * data is available.
 */
extern ssize_t ult_read(int fd, void* buf, size_t size);

#endif	/* ULT_H_INCLUDED */
