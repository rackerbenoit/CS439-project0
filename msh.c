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
#include <math.h>
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
 * Paul drove here
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
	    //Zoe drove here
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
        		sigemptyset(&mask);
        		sigaddset(&mask, SIGCHLD);
        		sigprocmask(SIG_BLOCK, &mask, NULL); // block
		
                if ((pid = Fork()) == 0)
                { // child
			         sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblock in child
			         // put child in new process group
			         if (setpgid(0, 0) < 0)
				            unix_error("setpgid error");	
			         // load the program
                     if (execve(argv[0], argv, environ) < 0 )
                     {
                            printf("%s: Command not found.\n", argv[0]);
                            exit(0);
                     }
                }

		//Paul did some driving here
                /* Parent waits for fg job to finish */
		addjob(jobs, pid, bg+1, cmdline);
		sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblock in parent

        //Enters if there is a foreground process
        if (!bg){	
		        waitfg(pid);
        }
        else // child is running in the bg
                printf("[%d] (%d) %s", (*getjobpid(jobs, pid)).jid, pid, cmdline);
        }

        return;				
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
//Zoe drove for this function
int builtin_cmd(char **argv) 
{
	if (!strcmp(argv[0], "quit")) /* quit command */
                exit(0);
	if (!strcmp(argv[0], "jobs")) /* lists all background jobs */
	{
		listjobs(jobs);	
		return 1;
	}
	if (!strcmp(argv[0], "bg"))	/* runs <job> in the bg */
	{
		do_bgfg(argv);	
		return 1;
	}
	if (!strcmp(argv[0], "fg"))	/* runs <job> in the fgh */
	{
		do_bgfg(argv);	
		return 1;
	}
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
    // check for no <job> argument
    if (argv[1] == NULL)
    {
	    fprintf(stdout, "%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    // see if the <job> contains letters	
    int i;
    for (i = 0; i < strlen(argv[1]); i++)
    {
	   if (isalpha(argv[1][i]))
	   {
	       fprintf(stdout, "%s: argument must be a PID or %%jobid\n", argv[0]);
	       return;
	   }
    }

    pid_t pid;
    char *firstCharPtr = argv[1]; // set ptr to start of <job> 
    char firstChar = *firstCharPtr; // get the first char
    char percent = '%'; //ascii value 37
    struct job_t *j;
    int isJID = 0; //FALSE

    // parse if is in the form of a JID
    if(firstChar == percent)
    {
        //fprintf(stdout, "Here now\n");
    	isJID = 1;
    	firstCharPtr++;
    	int jid = atoi(firstCharPtr);
        j = getjobjid(jobs, jid); //(pid_t)atoi(string);
    }
    else //If in form of pid
    {
        //fprintf(stdout, "Got here\n");
        pid = (pid_t)atoi(argv[1]);
    	j = getjobpid(jobs, pid);
    }


    // if the job does not exist
    if (j == NULL){
	   if (isJID) // if the JID is not valid
	   {
	       fprintf(stdout, "%s: No such job\n", argv[1]); 
	       return;
	   }
	   // else we were given an invalid PID
	   else
	       fprintf(stdout, "(%d): No such process\n", pid);
	   return;
    }

    pid = j->pid;

    //Zoe drove in the error hadling areas in this function
    if(!strcmp(argv[0], "bg"))
    {
        //fprintf(stdout, "bg job\n");
        if((*j).state == ST){
              if (kill(-pid, SIGCONT) < 0) 
		      unix_error("kill error");
        }
        (*j).state = BG;
        printf("[%d] (%d) %s", (*j).jid, pid, (*j).cmdline);
    }

    else if(!strcmp(argv[0], "fg"))
    {
        //fprintf(stdout, "fg job\n");
        if((*j).state == ST){
              if (kill(-pid, SIGCONT) < 0) 
		      unix_error("kill error");
        }

        (*j).state = FG;
	    waitfg(pid);
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
//Paul drove here
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
		idx = (getjobpid(jobs, pid))->jid;

		//Delete jobs if the job terminated
		if(WIFEXITED(status))
		{
			text = terminated;
			sig_int = WEXITSTATUS(status);
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
		if (WIFSTOPPED(status))
		{
			
		    struct job_t *currentJob = getjobpid(jobs, pid);
		    (*currentJob).state = ST; 
			text = stopped;
			sig_int = WSTOPSIG(status); // get signal
		}

        //If the child is stopped OR terminated b/c signal was ignored
		if(WIFSIGNALED(status) || WIFSTOPPED(status))
		{
		/*	char c1 = "Job [";
			char c2 = "] (";
			char c3 = ") ";
			char end = "\n";
			int length;	
			char *c;
			length = log10(idx) + 1;
			c = malloc(length);
			snprintf(c, length, "%d", idx);
			write(1, c, strlen(c));
		*/	
            char string[45];
			/* TODO: this prints out garbage on trace16, need to figure
			out another way to print this. maybe look into strcat or how 
			to convert ints to strings/chars? */ 		

            //int length = strlen(idx) + strlen(pid) + strlen(text) + strlen(sig_int);
			sprintf(string,"Job [%d] (%d) %s %d\n", idx, pid, text, sig_int);
            //memset(string, '\0', sizeof(string));
			bytes = write(STDOUT, string, 40);
			if(bytes != 40)
				exit(-999);
		}
	}	

	
	if (errno != ECHILD)
	{
		if (errno == EINTR)
		{
			Signal(sig, sigchld_handler);	
		}
		else
			unix_error("waitpid error");	
	}
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
    if(pid > 0)
    {
       if(kill(-pid, SIGINT) > 0) 
		unix_error("kill error");//Error handling
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
        if (kill(pid, SIGTSTP) < 0)
		unix_error("kill error");
    }


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



