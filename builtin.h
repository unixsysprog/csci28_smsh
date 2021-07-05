/*
 * ==========================
 *   FILE: ./builtin.h
 * ==========================
 * Purpose: Header file for builtin.c
 */

#ifndef	BUILTIN_H
#define	BUILTIN_H

int is_builtin(char **args, int *resultp);
int is_assign_var(char *cmd, int *resultp);
int is_list_vars(char *cmd, int *resultp);
int is_export(char **, int *);

int assign(char *);
int okname(char *);

// Added built-in functions
int is_exit(char **args, int *resultp);
int is_cd(char **args, int *resultp);
int is_read(char **args, int *resultp);

// Added variable substitution
char * varsub(char * args);

#endif
