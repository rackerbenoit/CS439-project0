#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);
pid_t Fork(void);

/*
 * unix_error - unix-style error routine.
 */
inline static void 
unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
  int arg;
  int print;

  if(argc != 2){
    fprintf(stderr, "Usage: fib <num>\n");
    exit(-1);
  }

  if(argc >= 3){
    print = 1;
  }

  arg = atoi(argv[1]);
  if(arg < 0 || arg > MAX){
    fprintf(stderr, "number must be between 0 and %d\n", MAX);
    exit(-1);
  }

  doFib(arg, 1);

  return 0;
}

/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void 
doFib(int n, int doPrint)
{
	int fib;
	// base case
	if (n == 0 || n == 1)
	{
		if (doPrint) fprintf(stdout, "%d\n", n);
		exit(n); 
	}

	pid_t pid;
	pid_t pid2;
	if ((pid = Fork()) == 0)		  
	{
		// child
		doFib(n - 1, 0);
	}

	if ((pid2 = Fork()) ==  0)
	{
		// child
		doFib(n - 2, 0);
	}

	if (pid > 0 && pid2 > 0)
	{
		// parent
		int status;
		int status2;
		waitpid(-1, &status, 0);
		waitpid(-1, &status2, 0);
		fib = WEXITSTATUS(status) + WEXITSTATUS(status2);
		if (doPrint)
		{
			fprintf(stdout, "%d\n", fib);
		}
		exit(fib);
	}
	
}

pid_t Fork(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		unix_error("Fork error");
	return pid;
}
