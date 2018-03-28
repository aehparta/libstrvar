/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRSLIST_H
#define _STRSLIST_H


/******************************************************************************/
/* INCLUDES */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ddebug/synchro.h>


/******************************************************************************/
/* STRUCTS */
struct var_sl
{
	char **l;
	size_t c;
	lock_t lock;
};
typedef struct var_sl * var_sl_t;


/******************************************************************************/
/* FUNCTION DEFINITIONS */
int var_sl_init(struct var_sl *);
int var_sl_add(struct var_sl *, const char *key, const char *string, ...);
void var_sl_del(struct var_sl *, int index);
void var_sl_cpe(struct var_sl *dst, struct var_sl *src);
void var_sl_cps(struct var_sl *dst, struct var_sl *src);
int var_sl_c(struct var_sl *);
char **var_sl_data(struct var_sl *);
void var_sl_free(struct var_sl *);
void var_sl_dump(FILE *f, struct var_sl *);
char *var_sl_at(struct var_sl *, int index);


#endif /* _STRSLIST_H */
/******************************************************************************/

