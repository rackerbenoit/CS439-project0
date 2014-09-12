#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */


//Paul is driving

void handler(int sig)
{
	ssize_t bytes; 
	const int STDOUT = 1; 
	bytes = write(STDOUT, "Nice try.\n", 10); 
	if(bytes != 10) 
   	exit(-999);	
}

void alarmhandler(int sig)
{

	struct timespec req;
	req.tv_sec = 1;
	struct timespec rem;
	req.tv_nsec = 0;

	int interrupt = nanosleep(&req, &rem); 
	printf("%s", "Still here\n");
	if(interrupt  == -1)
	{
		nanosleep(&rem, &rem);
	}	
}

int main(int argc, char **argv)
{
	/* print out this process's ID - Zoe is driving*/
	pid_t pid;
	pid = getpid();
	fprintf(stdout, "This process's ID is %d\n", pid);
	

	//Paul is driving
	Signal(SIGINT, handler);
	Signal(SIGALRM, alarmhandler);

	while(1){
		alarm(1);
	}
	return 0;
}


