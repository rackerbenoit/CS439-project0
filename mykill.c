#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

/* Takes a PID as an argument and sends
 * the SIGUSR1 signal to the specified
 * process ID. */

int main(int argc, char **argv)
{
	pid_t pid;
	pid = atoi(argv[1]);
	kill(pid, SIGUSR1);	
 	return 0;
}
