#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	"flexstr.h"

void	*emalloc(size_t);
void	*erealloc(void *, size_t);

/*
 * flexstr.c -- a set of functions for handling flexlists and flexstrings
 *
 *	a FLEXLIST is an array of strings that grows as needed
 *	methods are:  
 *
 *	fl_init(FLEXLIST *p,int chunk)	- initialize an FLEXLIST
 *	fl_append(FLEXLIST *p, char *)	- copy and add a str to a FLEXLIST
 *	fl_appendd(FLEXLIST *p, char *)	- add a str (no copy) to a FLEXLIST
 *	fl_free(FLEXLIST *p)		- dispose of all malloced data therein
 *	fl_getcount(FLEXLIST *p)	- return the number of items
 *
 *	char ** fl_getlist(FLEXLIST *p)	- return deep copy of list (NULL term)
 *	char ** fl_getlistd(FLEXLIST *p)- return the internal array
 *	void fl_freelist(char **l)	- free the deep copy of list(NULL term)
 *
 *      a FLEXSTR a string that grows as needed
 *
 *	fs_init(FLEXSTR *p,int chunk)
 *	fs_addch(FLEXSTR *p, char c)
 *	fs_addstr(FLEXSTR *p, char *str)
 *	fs_free(FLEXSTR *p)		- frees storage, resets counters
 *
 *	char *fs_getstr(FLEXSTR *p)	- rets copy of str, caller free()s it
 *	char *fs_getstrd(FLEXSTR *p)	- returns the internal string
 *
 *  VERSION 2: 10q to AL for mentioning V1's subtle complexity of memory mgmt
 *  2019-04-20: added fl_appendd to work with splitline
 *  2019-04-08: fl_getlist: do not call strdup on NULL (bug fix)
 *
 *  2019-04-20:
 *  TODO: return this to being a flexible list of pointers you pass it.
 *	  REMOVE strdups of the strings.  Let the caller allocate space
 *	  and let the fl_free function release them all unless the 
 *	  caller wants to.  Adding the strdups makes things worse.
 */



void 
fl_init(FLEXLIST *p, int amt)
{
	p->fl_list = NULL;
	p->fl_nslots = p->fl_nused = 0;
	p->fl_growby = ( amt > 0 ? amt : CHUNKSIZE );
}

int
fl_getcount(FLEXLIST *p)
{
	return p->fl_nused;
}

void
fl_free(FLEXLIST *p)
{
	int	i;

	for(i=0; i < p->fl_nused; i++)
		free(p->fl_list[i]);
	if ( p->fl_list )
		free(p->fl_list);
	fl_init(p,p->fl_growby);
}
/*
 * return a deep copy of the list.  Includes a NULL sentinel
 * returns NULL if strdup fails
 */
char **
fl_getlist(FLEXLIST *p)
{
	char	**rv = emalloc( (1+p->fl_nused) * sizeof(char *) );
	int	i;
	for( i = 0; i < p->fl_nused ; i++ ){
		if ( p->fl_list[i] != NULL ){
			rv[i] = strdup( p->fl_list[i] ) ;
			if ( rv[i] == NULL )
				return NULL;
		}
		else {
			rv[i] = NULL;
		}
	}
	rv[i] = NULL;
	return rv;
}

/*
 * returns the actual internal storage (faster, riskier)
 * no sentinel NULL
 */
char **
fl_getlistd(FLEXLIST *p)
{
	return p->fl_list;
}
/*
 * free a list returned by fl_getlist()
 */
void fl_freelist(char **list)
{
	int	i;
	for(i=0; list[i] != NULL ; i++ )
		free(list[i]);
	free(list);
}

/*
 * append string str to strlist, reallocing the array if needed
 * return 0 for ok dies on error
 * appendd - adds the string as given to array.  This must be
 *		dynamically allocated because it will be free()d later
 */

int
fl_appendd(FLEXLIST *p, char *str)
{
	if ( p->fl_nused == p->fl_nslots ){
		p->fl_nslots += p->fl_growby;
		p->fl_list    = erealloc(p->fl_list, 
					 p->fl_nslots * sizeof(char *));
	}
	p->fl_list[p->fl_nused++] = str;
	return 0;
}

/*
 * this version makes a copy of the string so you can reuse the buffer
 * you pass it.  It makes a new string and then calls fl_appendd
 * returns 0 or exits on error
 */
int
fl_append(FLEXLIST *p, char *str)
{
	return fl_appendd(p, (str != NULL ? strdup(str) : NULL ));
}

/************************************************************************
 *
 * flexstr functions
 *
 ************************************************************************/

void 
fs_init(FLEXSTR *p, int amt)
{
	p->fs_str = NULL;
	p->fs_space = p->fs_used = 0;
	p->fs_growby = ( amt > 0 ? amt : CHUNKSIZE );
}


/*
 * dispose of string and reset to empty 
 */
void
fs_free(FLEXSTR *p)
{

	free(p->fs_str);
	fs_init(p, p->fs_growby);
}

char *
fs_getstrd(FLEXSTR *p)
{
/* returns a pointer to the internal storage. faster/riskier than fs_getstr. */

    /* nul-terminate the string before returning it*/

    /* First make sure there's room for the '\0' */
    if (p->fs_used == p->fs_space)
    { /* string is full, add room for one more char */
        p->fs_str = erealloc(p->fs_str, ++p->fs_space);
    }

    /* Add terminating '\0'.  Don't increment fs_used -- the '\0' is not
     * part of the string, and shouldn't be counted if someone wants to
     * continue adding characters to the string later.
     */
    p->fs_str[p->fs_used] = '\0';

    /* Now return the (terminated) string. */
	return p->fs_str;
}
char *
fs_getstr(FLEXSTR *p)
{
/* wraps fs_getstrd to return a copy of the internal string. */

	return strdup( fs_getstrd(p) );
}

/*
 * append char to flexstring, reallocing the array if needed
 * return 0 for ok dies on error
 */

int
fs_addch(FLEXSTR *p, char c)
{
	if ( p->fs_space == 0 ){
		p->fs_str  = emalloc(p->fs_growby);
		p->fs_used = 0;
		p->fs_space= p->fs_growby;
	}
	else if ( p->fs_used == p->fs_space ){
		p->fs_space += p->fs_growby;
		p->fs_str    = erealloc(p->fs_str, p->fs_space);
	}
	p->fs_str[p->fs_used++] = c;
	return 0;
}

int
fs_addstr(FLEXSTR *p, char *s)
{
	int	c;

	while( (c = *s++) != '\0' )
		fs_addch(p, c);
	return 0;
}