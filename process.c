/*
 * ==========================
 *   FILE: ./process.c
 * ==========================
 * Purpose: command processing layer: handles layers of processing
 * 
 * The process(char **arglist) function is called by the main loop
 * It sits in front of the do_command function which sits 
 * in front of the execute() function.  This layer handles
 * two main classes of processing:
 *  a) process - checks for flow control (if, while, for ...)
 *  b) do_command - does the command by 
 *               1. Is command built-in? (exit, set, read, cd, ...)
 *                       2. If not builtin, run the program (fork, exec...)
 *
 * Most of this file has remained un-modified from the starter code. A few
 * lines were added in process() to handle for loop processing. In execute()
 * code has been added to convert the status returned from wait() to a proper
 * exit status.
 */

/* INCLUDES */
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <signal.h>
#include    <sys/wait.h>
#include    "smsh.h"
#include    "builtin.h"
#include    "varlib.h"
#include    "controlflow.h"
#include    "process.h"

int process(char *args[])
/*
 * purpose: process user command: this level handles flow control
 * returns: result of processing command
 *  errors: arise from subroutines, handled there
 */
{
    int     rv = 0;

    if (args[0] == NULL)   //just a new line
        rv = 0;
    else if ( is_control_command(args[0]) )
        rv = do_control_command(args);
    else if ( is_for_loop(args[0]) )            // added for assignment
        rv = do_for_loop(args);                 // added for assignment
    else if ( ok_to_execute() )
        rv = do_command(args);
        
    return rv;
}



/*
 * do_command
 *   purpose: do a command - either builtin or external
 *   returns: result of the command
 *    errors: returned by the builtin command or from exec,fork,wait
 *      note: this function was modified from starter code. Variable
 *            substitution was moved to builtin.c
 */
int do_command(char **args)
{
    int  rv;

    if ( is_builtin(args, &rv) )
        return rv;
    rv = execute(args);
    return rv;
}

int execute(char *argv[])
/*
 * purpose: run a program passing it arguments
 * returns: status returned via wait, or -1 on error
 *  errors: -1 on fork() or wait() errors
 *    note: this function was modified from the starter code to interpret
 *          the exit status received from wait to get the exit(n) status.
 *          This is set in the special variable $? back in smsh.c
 */
{
    extern char **environ;      /* note: declared in <unistd.h> */
    int pid ;
    int child_info = -1;
    int rv = -1;

    if ( argv[0] == NULL )      /* nothing succeeds     */
        return 0;

    if ( (pid = fork())  == -1 )
        perror("fork");
    else if ( pid == 0 ){
        environ = VLtable2environ();
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        execvp(argv[0], argv);
        perror("cannot execute command");
        exit(1);
    }
    else {
        if ( wait(&child_info) == -1 )
            perror("wait");
        
        // check/convert the exit status to the proper value
        if (WIFEXITED(child_info))
            rv = WEXITSTATUS(child_info);
        else if (WIFSIGNALED(child_info))
            rv = WTERMSIG(child_info);
    }
    return rv;
}
