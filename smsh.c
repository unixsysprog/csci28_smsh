/*
 * ==========================
 *   FILE: ./smsh.c
 * ==========================
 * Purpose: Provide interactive, or script-based, shell functionality.
 *
 * Outline: The functionality of the shell is split amongst several files,
 * each performing a specific task. This file contains the main loop which
 * calls on the other files to operate. For more information, see the Plan
 * document, or function comments in each of the files.
 *        splitline.c -- string I/O and management
 *          process.c -- execute programs
 *           varlib.c -- manage variables and the environment
 *      controlflow.c -- read if-blocks and for-loops
 *          builtin.c -- several built-in functions (cd, exit, etc.)
 */

/* INCLUDES */
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <signal.h>
#include    <sys/wait.h>
#include    <stdbool.h>
#include    "smsh.h"
#include    "splitline.h"
#include    "controlflow.h"
#include    "varlib.h"
#include    "process.h"
#include    "builtin.h"
#include    "flexstr.h"

/* CONSTANTS */
#define DFL_PROMPT  "> "

/* FILE-SCOPE VARIABLES */
static int last_exit = 0;
static int shell_mode = INTERACTIVE;
static int run_shell = 1;

/* INTERNAL FUNCTIONS */
static void run_command(char *);
static void execute_for();
static void setup();
static void io_setup();
static FILE * open_script(char *);

/*
 *  main()
 *  Purpose: Setup shell to be interactive, or run a script; then process
 *           command lines.
 *   Return: As commands are entered and processes run, the last exit status
 *           is populated in 'result'. When exit() is called, or shell reaches
 *           the EOF, that last exit status is returned (unless
 *           user-specified; see builtin.c).
 */
int main(int ac, char ** av)
{
    FILE * source;
    char *cmdline, *prompt;

    setup();    
    io_setup(&source, &prompt, ac, av);
    
    while ( run_shell )
    {
        cmdline = next_cmd(prompt, source);     // get next line from source
        
        if(cmdline == NULL)                     // cmdline was EOF
        {
            run_shell = safe_to_exit();         // check if processing if/for
            clearerr(stdin);                    // clear the EOF
            continue;
        }
        
        if( is_parsing_for() )                  // reading in a for_loop
        {
            if (load_for_loop(cmdline) == true) // when true
                execute_for();                  // for_loop complete, execute

            continue;                           // go to next cmdline
        }
        
        run_command(cmdline);                   // all other commands/syntax
    }
    
    return get_exit();
}

/*
 *  run_command()
 *  Purpose: Perform variable substitution and process() the command line
 *   Return: None; exit status result is updated in file-scope variable in
 *           this function.
 */
void run_command(char * cmdline)
{
    char *subline = varsub(cmdline);
    char **arglist;
    int result = 0;

    if ( (arglist = splitline(subline)) != NULL )
        result = process(arglist);
    
    if(result == -1)    // if command was a syntax error
        result = 2;     // change 2 to for correct exit status

    set_exit(result);   // update $? value
    return; 
}

/*
 *  execute_for()
 *  Purpose: Iterate through a completed for_loop struct and execute cmds
 *   Return: None; this function loads the for loop struct, executes all
 *           commands, and updates $? value.
 */
void execute_for()
{
    char **vars = get_for_vars();           // load in varvalues
    char **cmds = get_for_commands();       // load in commands
    char * name = get_for_name();           // load in varname for sub
    char ** cmds_start;

    while(*vars)                            // for each varvalue
    {
        if (VLstore(name, *vars) == 1)      // set current var for sub
        {
            fprintf(stderr, "Problem updating the for variable. \n");
            set_exit(2);
            return;
        }
        
        cmds_start = cmds;                  // reset to first command
        
        while(*cmds_start)                  // go through cmds for each var
            run_command(*cmds_start++);     // execute
            
        vars++;                             // next variable
    }
    
    free(name); 
    return;
}

void setup()
/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
{
    extern char **environ;

    VLenviron2table(environ);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

/*
 *  io_setup
 *  Purpose: Detect if smsh should be run in interactive, or script mode.
 *           Set 'FILE*' and 'prompt' accordingly.
 *    Input: fp, address of FILE * back in main
 *           pp, address of pointer to "prompt" back in main
 *           args, number of command-line args
 *           av, command-line args
 *   Return: none
 *   Errors: If a file is specified, but cannot be opened, open_script()
 *           will output a message and exit.
 */
void io_setup(FILE ** fp, char ** pp, int args, char ** av)
{
    if(args >= 2)
    {
        *fp = open_script(av[1]);
        *pp = "";
        shell_mode = SCRIPTED;
    }
    else
    {
        *fp = stdin;
        *pp = DFL_PROMPT;
    }
    
    return;
}

/*
 *  open_script()
 *  Purpose: Open a file, and handle any errors it encounters.
 *   Return: Pointer to the file (shell script) it opened
 */
FILE * open_script(char * file)
{
    FILE * fp = fopen(file, "r");
    
    if(fp == NULL)
    {
        fprintf(stderr, "Can't open %s\n", file);
        exit(127);
    }
    
    return fp;
}

/*
 *  get_exit() -- getter function to access $? value
 */
int get_exit()
{
    return last_exit;
}

/*
 *  set_exit() -- setter function to update $? value
 */
void set_exit(int status)
{
    last_exit = status;
    return;
}

/*
 *  get_mode()
 *  Purpose: getter function to check if shell is in INTERACTIVE or
 *           SCRIPTED mode
 */
int get_mode()
{
    return shell_mode;
}

/*
 *  fatal() -- Helper function to display err and terminate shell
 */
void fatal(char *s1, char *s2, int n)
{
    fprintf(stderr,"Error: %s,%s\n", s1, s2);
    exit(n);
}
