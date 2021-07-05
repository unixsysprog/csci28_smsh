/*
 * ==========================
 *   FILE: ./controlflow.h
 * ==========================
 * Purpose: Header file for controlflow.c
 */
 
// From starter code
int is_control_command(char *);
int do_control_command(char **);
int ok_to_execute();

// To call in process.c
int is_for_loop(char *s);
int do_for_loop(char **args);

// To call in smsh.c
int load_for_loop(char *args);
int is_parsing_for();
int safe_to_exit();

// getter functions
char ** get_for_commands();
char ** get_for_vars();
char * get_for_name();