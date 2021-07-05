/*
 * ==========================
 *   FILE: ./controlflow.c
 * ==========================
 * Purpose: Handle if-block and for-loop control syntax.
 *
 * "if" processing is done with two state variables
 *    if_state and if_result
 *
 *  Else block handling for if/then/fi control was added for the assignment.
 *  A for loop data structure and several additional helper functions were
 *  also added. For existing functions that were modified, see in-line
 *  comments for the changes. For new functions, please reference header
 *  comments above the function.
 */
 
/* INCLUDES */
#include    <stdio.h>
#include    <string.h>
#include    <stdbool.h>
#include    <stdlib.h>
#include    "smsh.h"
#include    "controlflow.h"
#include    "splitline.h"
#include    "process.h"
#include    "builtin.h"
#include    "flexstr.h"
#include    "varlib.h"

/* FOR LOOP STRUCTURE */
struct for_loop {
    FLEXSTR varname;        // variable name
    FLEXLIST varvalues;     // list of values after 'in'
    FLEXLIST commands;      // list of commands between 'do' and 'done'
};

static struct for_loop fl;  // file-scope struct to store a for loop

/* CONTROL STATE VARIABLES */
enum states   { NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK,
                WANT_DO, WANT_DONE };
enum results  { SUCCESS, FAIL };

/* FILE-SCOPE VARIABLES */
static int for_state = NEUTRAL;
static int if_state  = NEUTRAL;
static int if_result = SUCCESS;
static int last_stat = 0;

/* INTERNAL FUNCTIONS */
static int syn_err(char *);
static int init_for_loop(char **);
static void load_for_varname(char *);
static void load_for_varvalues(char **);

int ok_to_execute()
/*
 * purpose: determine the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details: if in THEN_BLOCK and if_result was SUCCESS then yes
 *          if in THEN_BLOCK and if_result was FAIL    then no
 *          if in WANT_THEN  then syntax error (sh is different)
 *   notes: Copied from starter-code for assignment 5. Code added to handle
 *          else blocks.
 */
{
    int rv = 1;     /* default is positive */

    if ( if_state == WANT_THEN ){
        syn_err("then expected");
        rv = 0;
    }
    else if ( if_state == THEN_BLOCK && if_result == SUCCESS )
        rv = 1;
    else if ( if_state == THEN_BLOCK && if_result == FAIL )
        rv = 0;
    else if ( if_state == ELSE_BLOCK && if_result == SUCCESS )  // added code
        rv = 0;
    else if ( if_state == ELSE_BLOCK && if_result == FAIL )     // added code
        rv = 1;
    return rv;
}



int is_control_command(char *s)
/*
 * purpose: boolean to report if the command is a shell control command
 * returns: 0 or 1
 *   notes: Copied from starter-code for assignment 5. Code added to handle
 *          else blocks.
 */
{
    return (strcmp(s, "if") == 0 ||
            strcmp(s, "then") == 0 ||
            strcmp(s, "else") == 0 ||
            strcmp(s, "fi") == 0);
}

/*
 *  is_for_loop()
 *  Purpose: boolean to report if the command is a for loop command
 *   Return: 0 or 1
 *     Note: This function mimics the is_control_command() included in the
 *           starter code; just with the for loop keywords.
 */
int is_for_loop(char *s)
{
    return (strcmp(s, "for") == 0 ||
            strcmp(s, "do") == 0 ||
            strcmp(s, "done") == 0);
}

/*
 *  load_for_loop()
 *  Purpose: Once a for loop has been started, load_for_loop() is called until
 *           'done' to populate for loop struct.
 *   Return: true, when done loading for loop
 *           false, otherwise
 */
int load_for_loop(char *args)
{   
    char **arglist = splitline(args);
    
    if(arglist == NULL || arglist[0] == NULL)   // check if we have args
        return false;                           // we don't
    
    if(for_state == WANT_DO)
    {
        if(strcmp(arglist[0], "do") == 0)
            for_state = WANT_DONE;
        else
            return syn_err("word unexpected (expecting \"do\")");
    }
    else if (for_state == WANT_DONE)
    {
        if(strcmp(arglist[0], "done") == 0) // reached the end?
        {
            for_state = NEUTRAL;            // reset state
            return true;                    // done loading
        }

        fl_append(&fl.commands, args);      // not a 'done', load raw command
    }
    else
        fatal("internal error processing:", arglist[0], 2);
    
    return false;
}

/*
 *  do_for_loop()
 *  Purpose: Process "for", "do", "done" - start loading for loop at start;
 *           display errors for out-of-order commands
 *   Return: 0 if ok, -1 (or fatal) for syntax error
 */
int do_for_loop(char **args)
{
    char *cmd = args[0];
    int rv = -1;
    
    if (strcmp(cmd, "for") == 0)
    {
        if (for_state != NEUTRAL)
            rv = syn_err("for unexpected");
        else 
        {
            rv = init_for_loop(args);       // start loading for loop
            for_state = WANT_DO;
        }
    }
    // check for out-of-sequence control words
    else if (strcmp(cmd, "do") == 0)
    {
        if (for_state != WANT_DO)
            rv = syn_err("do unexpected");
    }
    else if (strcmp(cmd, "done") == 0)
    {
        if (for_state != WANT_DONE)
            rv = syn_err("done unexpected");
    }
    else
        fatal("internal error processing:", cmd, 2);
        
    return rv;
}

int do_control_command(char **args)
/*
 * purpose: Process "if", "then", "else", "fi" - change state or detect error
 * returns: 0 if ok, -1 for syntax error
 *   notes: I would have put returns all over the place, Barry says "no"
 *   notes: Copied from starter-code for assignment 5. Code added to handle
 *          else blocks.
 */
{
    char    *cmd = args[0];
    int rv = -1;

    if( strcmp(cmd,"if")==0 ){
        if ( if_state != NEUTRAL )
            rv = syn_err("if unexpected");
        else {
            last_stat = process(args+1);
            if_result = (last_stat == 0 ? SUCCESS : FAIL );
            if_state = WANT_THEN;
            rv = 0;
        }
    }
    else if ( strcmp(cmd,"then")==0 ){
        if ( if_state != WANT_THEN )
            rv = syn_err("then unexpected");
        else {
            if_state = THEN_BLOCK;
            rv = 0;
        }
    }
    // added for the assignment
    else if ( strcmp(cmd, "else") == 0) {
        if( if_state != THEN_BLOCK )
            rv = syn_err("else unexpected");
        else {
            if_state = ELSE_BLOCK;
            rv = 0;
        }
    }
    // modified for the assignment
    else if ( strcmp(cmd,"fi")==0 ){
        if ( if_state == THEN_BLOCK || if_state == ELSE_BLOCK) {
            if_state = NEUTRAL;
            rv = 0;
        }
        else {
            rv = syn_err("fi unexpected");
        }
    }
    else 
        fatal("internal error processing:", cmd, 2);
    return rv;
}



/*
 *  init_for_loop()
 *  Purpose: Check the first line of for loop syntax, and initialize struct
 *   Return: 0 on success, -1 (or fatal) on syntax error
 */
int init_for_loop(char **args)
{   
    args++;                             //strip the 'for'
    
    if ( okname(*args) )                // valid varname
    {
        load_for_varname(*args++);      // store in struct, strip from args
        
        if(strcmp(*args, "in") == 0)    // validate "in"
        {
            load_for_varvalues(++args); // load any args after that
            for_state = WANT_DO;        // change state
        }
        else
        {
            return syn_err("word unexpected (expecting \"in\")");
        }
    }
    else
    {
        syn_err("Bad for loop variable");
    }
    
    return 0;
}

/*
 *  is_parsing_for()
 *  Purpose: Check if shell is currently reading in a for_loop struct
 *   Return: 1 if reading in a for_loop, 0 if not
 */
int is_parsing_for()
{
    return for_state != NEUTRAL;
}

/*
 *  safe_to_exit()
 *  Purpose: On EOF, check if it is safe for the shell to exit.
 *   Return: 0 when okay to exit, -1 (or fatal) from syn_err otherwise
 *     Note: If the shell is currently processing an if-block or a for loop,
 *           the shell will output an error message, and reset the state of
 *           the control-flow, a la 'dash'. set_exit() is called to ensure
 *           exit status of 2 for syntax errors.
 */
int safe_to_exit()
{
    if (if_state != NEUTRAL || for_state != NEUTRAL)
    {
        set_exit(2);
        return syn_err("end of file unexpected");
    }
    
    return 0;
}

int syn_err(char *msg)
/* purpose: handles syntax errors in control structures
 * details: resets state to NEUTRAL
 * returns: -1 in interactive mode. Should call fatal in scripts
 */
{
    if(get_mode() == SCRIPTED)
        fatal("syntax error: ", msg, 2);
        
    if_state = NEUTRAL;
    for_state = NEUTRAL;
    fprintf(stderr,"syntax error: %s\n", msg);

    return -1;
}


/*
 *  load_for_varname()
 *  Purpose: Helper function to initialize varname field in for loop struct
 *    Input: str, the value of varname
 */
void load_for_varname(char * str)
{
    FLEXSTR name;
    fs_init(&name, 0);
    fs_addstr(&name, str);
    fs_addch(&name, '\0');
    fl.varname = name;
}

/*
 *  load_for_varvalues()
 *  Purpose: Helper function to initialize varvalues field in for loop struct
 *    Input: args, array of variable values
 */
void load_for_varvalues(char **args)
{
    FLEXLIST vars;
    fl_init(&vars, 0);
    while(*args)
    {
        fl_append(&vars, *args);
        args++;
    }
    
    fl.varvalues = vars;
}

/*
 *  get_for_commands()
 *  Purpose: getter to access for struct info in main()
 */
char ** get_for_commands()
{
    return fl_getlist(&fl.commands);
}

/*
 *  get_for_vars()
 *  Purpose: getter to access for struct info in main()
 */
char ** get_for_vars()
{
    return fl_getlist(&fl.varvalues);
}

/*
 *  get_for_name()
 *  Purpose: getter to access for struct info in main()
 */
char * get_for_name()
{
    return fs_getstr(&fl.varname);
}
