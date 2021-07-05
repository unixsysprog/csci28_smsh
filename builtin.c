/*
 * ==========================
 *   FILE: ./builtin.c
 * ==========================
 * Purpose: contains the switch and the functions for builtin commands
 *
 * Copied from starter code. is_builtin() was modified to includes tests
 * for the new built-in functions written for the assignment. Other
 * functions were unmodified. New functions are as follows:
 *      is_exit()         -- Terminate shell
 *      is_cd()           -- Change directories
 *      is_read()         -- Assign input from stdin to a variable
 *      varsub()          -- Do variable substitution
 * The following are internal helper functions:
 *      get_replacement() -- Get string to replace a $VARIABLE
 *      get_special()     -- Convert a number to a string
 *      get_var()         -- Extract a valid variable name, to be replaced
 *      get_number()      -- Helper function to check if str is a number
 */

/* INCLUDES */
#include    <stdio.h>
#include    <string.h>
#include    <errno.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <stdbool.h>
#include    "smsh.h"
#include    "varlib.h"
#include    "flexstr.h"
#include    "splitline.h"
#include    "builtin.h"

/* INTERNAL FUNCTIONS */
static char * get_replacement(char * args, int * len);
static char * get_special(int val);
static char * get_var(char *args, int * len);
static int get_number(char * str);


int is_builtin(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
    if ( is_exit(args, resultp) )           // added for assignment
        return 1;
    if ( is_assign_var(args[0], resultp) )
        return 1;
    if ( is_list_vars(args[0], resultp) )
        return 1;
    if ( is_export(args, resultp) )
        return 1;
    if ( is_cd(args, resultp) )             // added for assignment
        return 1;
    if ( is_read(args, resultp) )           // added for assignment
        return 1;
    return 0;
}

/* checks if a legal assignment cmd
 * if so, does it and retns 1
 * else return 0
 */
int is_assign_var(char *cmd, int *resultp)
{
    if ( strchr(cmd, '=') != NULL ){
        *resultp = assign(cmd);
        if ( *resultp != -1 )
            return 1;
    }
    return 0;
}

/* checks if command is "set" : if so list vars */
int is_list_vars(char *cmd, int *resultp)
{
    if ( strcmp(cmd,"set") == 0 ){       /* 'set' command? */
        VLlist();
        *resultp = 0;
        return 1;
    }
    return 0;
}

/*
 * if an export command, then export it and ret 1
 * else ret 0
 * note: the opengroup says
 *  "When no arguments are given, the results are unspecified."
 */
int is_export(char **args, int *resultp)
{
    if ( strcmp(args[0], "export") == 0 ){
        if ( args[1] != NULL && okname(args[1]) )
            *resultp = VLexport(args[1]);
        else
            *resultp = 1;
        return 1;
    }
    return 0;
}

/*
 *  is_cd()
 *  Purpose: change directories
 *    Input: args, command line arguments
 *           resultp, where to store result of cd operation
 *   Return: 1 if built-in function, 0 otherwise. resultp is 0
 *           if chdir() was successful, 2 on (syntax) error.
 */
int is_cd(char **args, int *resultp)
{
    if ( strcmp(args[0], "cd") == 0 )
    {
        if (args[1] != NULL)
            *resultp = chdir(args[1]);              // go to dir specified
        else
            *resultp = chdir(VLlookup("HOME"));     // go to HOME directory
        
        
        if( *resultp == -1)                         // chdir failed
        {
            fprintf(stderr, "cd: %s: %s\n",
                            args[1], strerror(errno));
            *resultp = 2;                           // syntax error
        }
        return 1;                                   //was a built-in
    }
    return 0;
}


/*
 *  is_exit()
 *  Purpose: Terminate the shell process.
 *    Input: args, command line arguments
 *           resultp, where to store result of exit operation
 *   Return: If there is a syntax error, resultp is assigned a value of 2
 *           and is_exit() returns with 1 to indicate 'exit' was called.
 *           Otherwise, exit() is called with the status specified on the
 *           command line, or with the exit status of the preceding command.
 */
int is_exit(char **args, int *resultp)
{
    if( strcmp(args[0], "exit") == 0)
    {
        if( args[1] != NULL )               // exit arg specified?
        {
            int val = get_number(args[1]);  // convert to a number

            if (val != -1)                  // successful?
                exit(val);                  // use it
            else                            // syntax error
            {
                fprintf(stderr, "exit: Illegal number: %s\n", args[1]);
                *resultp = 2;
            }
        }
        else
        {
            exit( get_exit() );         //exit with last command's status   
        }
        
        return 1;                       //was a built-in
    }
    return 0;
}

/*
 *  is_read()
 *  Purpose: Assign input from stdin to the name of a specified variable
 *    Input: args, command line arguments
 *           resultp, where to store result of read operation
 *   Return: result
 */
int is_read(char **args, int *resultp)
{    
    if ( strcmp(args[0], "read") == 0)
    {
        if( args[1] != NULL && okname(args[1]) ) // check if a valid var name
        {           
            char * str = next_cmd("", stdin);    // next_cmd reads until '\n'
            *resultp = VLstore(args[1], str);
        }
        else                                     // syntax error
        {
            fprintf(stderr, "read: %s: bad variable name\n", args[1]);
            *resultp = 2;
        }
        return 1;
    }
    
    return 0;
}

int assign(char *str)
/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but retores it to normal
 */
{
    char    *cp;
    int rv ;

    cp = strchr(str,'=');
    *cp = '\0';
    rv = ( okname(str) ? VLstore(str,cp+1) : -1 );
    *cp = '=';
    return rv;
}

int okname(char *str)
/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 */
{
    char    *cp;

    for(cp = str; *cp; cp++ ){
        if ( (isdigit(*cp) && cp==str) || !(isalnum(*cp) || *cp=='_' ))
            return 0;
    }
    return ( cp != str );   /* no empty strings, either */
}

#define is_delim(x) ((x)==' '|| (x)=='\t' || (x)=='\0')

/*
 *  varsub()
 *  Purpose: Check line for escape-chars, comments, and any variable subs
 *    Input: args, un-modified cmdline input
 *   Return: a copy of the modified cmdline
 *   Method: If not NULL, varsub() iterates over every char in the cmdline
 *           that is passed in. If it does not match a special case - either
 *           a comment (#), a literal-next (\), or a variable ($) - the char
 *           is untouched. For the special cases, a comment is valid if the
 *           previous character is whitespace. Here, the rest of the line is
 *           ignored. For a literal next, if a next char exists, it is output
 *           as-is; if the '\' is the last char, the backslash is output
 *           (note: most shells would traditional treat a backslash at the
 *           end of a line to look for continuation of input on the next line.
 *           As noted in Piazza post @282, this is not required for the
 *           assignment). For variable substitution, it calls on
 *           get_replacement() to get a string to append. For more info on
 *           how that works, see header comments below.
 */
char * varsub(char * args)
{
    int skipped;
    char c, prev;
    char *newstr, *retval;
    
    if (args == NULL)
        return NULL;

    FLEXSTR s;
    fs_init(&s, 0);
    
    while ( (c = args[0]) )                     // go through cmdline
    {
        if (c == '#' && is_delim(prev) )        // start of comment
            break;                              // ignore the rest
        else if (c == '\\' && args[1])          // escape char
        {
            fs_addch(&s, args[1]);              // add the literal next
            args++;
        }
        else if (c == '$')                      // variable sub
        {
            newstr = get_replacement(++args, &skipped);
            args += (skipped - 1);              // -1 because args++ below
            fs_addstr(&s, newstr);
        }
        else                                    // regular char
            fs_addch(&s, c);                    // add as-is
        
        prev = c;
        args++;
    }
    
    fs_addch(&s, '\0');                         // terminate string
    retval = fs_getstr(&s);                     // get a copy of the string
    fs_free(&s);                                // release fs memory
    return retval;
}

/*
 *  get_replacement()
 *  Purpose: Return a string that will replace a $VARIABLE in a command line
 *    Input: args, the command line from the start of the variable to sub
 *           len, pointer the varsub uses to know where the end of the
 *                is located
 *   Return: String to substitute in place for the $VARIABLE
 *   Method: First, call get_var() to get the 'name' of the valid variable
 *           to replace. get_var() reads in chars until it finds a non-
 *           alpha-numeric or underscore char. It then evaluates the variable
 *           and performs a varlib lookup, or calls get_special() for the $$
 *           or $? variables.
 */
char * get_replacement(char * args, int * len)
{
    // get the variable to replace
    char * to_replace = get_var(args, len);     //++args to trim '$' from head
    char *retval;
    
    if (strcmp(to_replace, "$") == 0)           // special PID var
        retval = get_special(getpid());
    else if (strcmp(to_replace, "?") == 0)      // special exit-status var
        retval = get_special(get_exit());
    else                                        // environment var
        retval = VLlookup(to_replace);
        
    free(to_replace);
    return retval;
}

/*
 *  get_special()
 *  Purpose: Convert a number to a string
 *    Input: val, the value of a special variable $$ or $?
 *   Return: A stringified version of the special value passed in
 */
char * get_special(int val)
{
    char special[sizeof(pid_t) * sizeof(int)] = "";

    if (sprintf(special, "%d", val) < 0)        // sprintf error
        return "";                              // return empty string

    FLEXSTR var;
    fs_init(&var, 0);
    fs_addstr(&var, special);                   // number was converted
    fs_addch(&var, '\0');                       // terminate
    
    char *retval = fs_getstr(&var);             // get a copy of the string
    fs_free(&var);                              // release fs memory
    return retval;  
}

/*
 *  get_var()
 *  Purpose: Extract a valid variable name, to be replaced
 *    Input: args, the command line from the start of the variable to sub
 *           len, pointer the varsub uses to know where the end of the
 *                is located
 *   Return: String the contains name of variable to be replaced.
 *     Note: The code always adds one char to the var string. While this
 *           works for most cases, and ensures special variables can work,
 *           the solution is not ideal. It is extra code that could probably
 *           be incorporated into the while loop, and it does not catch error
 *           condition that variable names cannot begin with a digit (for
 *           this shell assignment).
 */
char * get_var(char *args, int * len)
{
    char c;
    int skipped = 0;
    FLEXSTR var;
    fs_init(&var, 0);
    fs_addch(&var, args[0]);        //add at least one char (could be $ or ?)
    skipped++;
    args++;
    
    while ( (c = args[0]) )
    {
        if( isalnum(c) || c == '_')             // valid?
            fs_addch(&var, c);                  // add it
        else
            break;                              // stop
        
        skipped++;
        args++;
    }
    
    fs_addch(&var, '\0');                   // terminate
    *len = skipped;                         // pass back position
    char *retval = fs_getstr(&var);         // get a copy of the string
    fs_free(&var);                          // release fs memory
    return retval;
}



/*
 *  get_number()
 *  Purpose: Helper function to check if str is a number
 *    Input: str, the string to check
 *   Return: -1 on error (str is not a number)
 *           value returned from atoi() on success
 */
int get_number(char * str)
{
    int i;
    int len = strlen(str);
    
    for(i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
            return -1;
    }
    
    return atoi(str);
}
