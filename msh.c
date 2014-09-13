/* 
 * msh - A mini shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/*
 * Fork - fork() wrapper
 *
 */
pid_t Fork(void)
{
        pid_t pid;
        if ((pid = fork()) < 0)
                unix_error("Fork error");
        return pid;
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
	char *argv[MAXARGS];
        char buf[MAXLINE];
        int bg;                 /* run in bg or fg? */
        pid_t pid;
	sigset_t mask;

        strcpy(buf, cmdline);
        bg = parseline(buf, argv);
        if (argv[0] == NULL)
                return;         /* Ignore empty lines */

	// Is not a built-in command
        if (!builtin_cmd(argv))
        {
		sigprocmask(SIG_BLOCK, &mask, NULL); // block
                if ((pid = Fork()) == 0)
                { // child
			sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblock in child
			// put child in new process group
			if (setpgid(0, 0) < 0)
			{
				unix_error("setpgid error");		
			}
			// load the program
                        if (execv(argv[0], argv) < 0 )
                        {
                                printf("%s: Command not found.\n", argv[0]);
                                exit(0);
                        }
                }


                /* Parent waits for fg job to finish */
		addjob(jobs, pid, bg+1, cmdline);
		sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblock in parent
                if (!bg)
                {
			waitfg(pid);
                }
                else // child is running in the bg
                        printf("%d Process Running: %s", pid, cmdline);
        }
        return;				
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
	if (!strcmp(argv[0], "quit")) /* quit command */
                exit(0);
	if (!strcmp(argv[0], "jobs")) /* lists all background jobs */
		listjobs(jobs);	
	if (!strcmp(argv[0], "bg"))	/* runs <job> in the bg */
		do_bgfg(argv);	
	if (!strcmp(argv[0], "fg"))	/* runs <job> in the fgh */
		do_bgfg(argv);	
        if (!strcmp(argv[0], "&")) /* Ignore singleton & */
                return 1;	
	return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 * Paul is driving
 */
void do_bgfg(char **argv) 
{
    pid_t pid;
    char *string = argv[1];
    char *firstChar = "";
    sprintf(firstChar, "%c", *string);

    //if jid is passed in
    if(strcmp(firstChar, "%"))
    {
        string++;
        pid = (pid_t)atoi(string);
    }
    //If pid is passed in
    else
    {
        pid = (pid_t)atoi(argv[1]);
    }

    struct job_t *currentJob = getjobpid(jobs, pid);
    if(strcmp(argv[0], "bg"))
    {
        if((*currentJob).state == ST){
            if (kill(-pid, SIGCONT) < 0) //TODO This should continue all processes in this pid group?
		unix_error("kill error");
        }
        (*currentJob).state = BG;
    }

    else if(strcmp(argv[0], "fg"))
    {
        if((*currentJob).state == ST){
            if (kill(-pid, SIGCONT) < 0) //TODO Continue process if stopped
		unix_error("kill error");
        }

        (*currentJob).state = FG;

    }

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
	// while the pid is the fg process sleep the parent process
	while (fgpid(jobs) == pid)
	{
		sleep(1);
	}
	return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) /* Zoe driving here */
{
	pid_t pid;
	int status;

	// reap all terminated children, check for stopped children
	while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0)
	{
		int idx, sig_int;
		char terminated[] = "terminated by signal";
		char stopped[] = "stopped by signal";
		char *text;
	    	ssize_t bytes;
	    	const int STDOUT = 1;

		//Delete jobs if the job terminated
		if(WIFEXITED(status))
		{
		    deletejob(jobs, pid);
		}
		
		// if child terminated b/c signal was not caught
		if (WIFSIGNALED(status))
		{
			text = terminated;
			sig_int = WTERMSIG(status); // get signal
            		deletejob(jobs, pid); //Signals we cannot handle
		}
		// if the child is stopped
		else if (WIFSTOPPED(status))
		{
			text = stopped;
			sig_int = WSTOPSIG(status); // get signal
		}

		char string[45];
		sprintf(string,"Job [%d] (%d) %s %d", idx, pid, text, sig_int);
		bytes = write(STDOUT, string, 45);
		if(bytes != 45)
			exit(-999);
		exit(1);
	}	

	
	if (errno != ECHILD)
		unix_error("waitpid error");	
	return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
 //Paul is Driving
void sigint_handler(int sig) 
{
    pid_t pid = fgpid(jobs);

    //Kills foreground job is there a foreground job running
    if(pid != 0)
    {
        if (kill(-pid, SIGINT) < 0)
		unix_error("kill error");
        deletejob(jobs, pid);
    }

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
 //Paul is Driving
void sigtstp_handler(int sig) 
{
    pid_t pid = fgpid(jobs);

    //If foreground job exists, suspend it
    if(pid != 0)
    {
        if (kill(-pid, SIGTSTP) < 0)
		unix_error("kill error");
    }

    struct job_t *currentJob = getjobpid(jobs, pid);
    (*currentJob).state = ST; 

    return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}



