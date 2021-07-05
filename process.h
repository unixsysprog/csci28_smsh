/*
 * ==========================
 *   FILE: ./process.h
 * ==========================
 * Purpose: Header file for process.c
 *
 * Note: All the following code and comments are copied from the
 *		 CSCI-28 course website, without modification.
 */

#ifndef	PROCESS_H
#define	PROCESS_H

int process(char **args);
int do_command(char **args);
int execute(char **args);

#endif
