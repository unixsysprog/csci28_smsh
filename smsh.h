/*
 * ==========================
 *   FILE: ./smsh.h
 * ==========================
 * Purpose: Header file for smsh.c
 */

#ifndef	SMSH_H
#define	SMSH_H

enum mode { INTERACTIVE, SCRIPTED };

int get_exit();
void set_exit(int);
int get_mode();
void fatal(char *, char *, int);

#endif