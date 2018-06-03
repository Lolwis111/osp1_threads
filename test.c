#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "ult.h"

unsigned long long int bytesTransfered;
float timePassed;

static void threadA()
{
	/* open some filestreams */
	int infile = open("/dev/random", O_RDONLY);
	int outfile = open("/dev/null", O_RDWR);

	/* get the starting time */
	clock_t startTime = clock();
	while(1)
	{
		/* read one byte (if available) and write it out */
		char buf;
		ult_read(infile, &buf, sizeof(buf));
		write(outfile, &buf, sizeof(buf));

		/* build some stats */
		bytesTransfered++;
		timePassed = (float)(clock() - startTime) / CLOCKS_PER_SEC;

		ult_yield();
	}

	ult_exit(0);
}

static void threadB()
{
	while(1) 
	{
		char buffer[16];
		memset(buffer, 0, sizeof(buffer));
		ssize_t size;
		/* read (non blocking) a command from the user */
		if((size = ult_read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
		{
			/* print some stats */
			if(0 == strncmp(buffer, "stats\n", 6))
			{
				printf("Datenmenge: ");

				if(bytesTransfered > 1000000)
					printf("%f MByte\n", ((float)bytesTransfered) / (1024 * 1024));
				else if(bytesTransfered > 1000)
					printf("%f KByte\n", ((float)bytesTransfered) / 1024);
				else
					printf("%llu Byte\n", bytesTransfered);

				printf("Laufzeit:   %f s\n", timePassed);

				float speed = ((float)bytesTransfered / timePassed);
				
				printf("Durchsatz:  ");
				if(speed > 1000000) 
					printf("%f MByte/s\n", speed / (1024 * 1024));
				else if(speed > 1000) 
					printf("%f KByte/s\n", speed / 1024);
				else 
					printf("%f Byte/s\n", speed);
			}
			/* exit the threadtool */
			else if(0 == strncmp(buffer, "exit\n", 5))
			{
				exit(EXIT_SUCCESS);
			}
			else
			{
				puts("Command not found!\n");
			}
		}

		ult_yield();
	}

	ult_exit(0);
}

static void myInit()
{
	int tids[2], i, status;
	
	/* spawn the two threads */
	printf("spawn A\n");
	tids[0] = ult_spawn(threadA);
	printf("spawn B\n");
	tids[1] = ult_spawn(threadB);
	
	for (i = 0; i < 2; ++i)
	{
		printf("waiting for tids[%d] = %d\n", i, tids[i]);
		fflush(stdout);
		
		/* wait for both to finish */
		if (ult_join(tids[i], &status) < 0)
		{
			fprintf(stderr, "join for %d failed\n", tids[i]);
			ult_exit(-1);
		}
		
		printf("(status = %d)\n", status);
	}
	
	ult_exit(0);
}

int main()
{
	ult_init(myInit);
	return 0;
}


