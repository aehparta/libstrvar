/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRLLIST_H
#define _STRLLIST_H


/******************************************************************************/
/* INCLUDES */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ddebug/synchro.h>
#include <ddebug/debuglib.h>
#include "strvar.h"


/******************************************************************************/
/* STRUCTS */
struct var_llist_item
{
	int type;
	void *data;
	size_t size;
	struct var_llist_item *next;
};
struct var_llist
{
	struct var_llist_item *first;
	struct var_llist_item *last;
	struct var_llist_item *current;
	size_t count;
	lock_t lock;
};
typedef struct var_llist * linkedl_t;
typedef struct var_llist * var_ll_t;


/******************************************************************************/
/* FUNCTION DEFINITIONS */

/**
 * Create new linked list.
 *
 * @return New linked list, or NULL on errors.
 */
linkedl_t var_ll_new(void);

/**
 * Append new string at end of the list.
 *
 * @param list Linked list.
 * @param str String to append.
 */
int var_ll_app(linkedl_t list, const char *string, ...);

/**
 * Append new binary data at end of the list.
 *
 * @param list Linked list.
 * @param data Pointer to data.
 * @param len Data length.
 */
int var_ll_app_bin(linkedl_t list, void *data, size_t len);

/**
 * Append new pointer at end of the list.
 */
int var_ll_app_p(linkedl_t list, void *pointer);

/**
 * Append new integer at end of the list.
 */
int var_ll_app_int(linkedl_t list, int value);

/*
void var_ll_pre(linkedl_t list, const char *str);
void var_ll_ins(linkedl_t list, const char *str, int index);
*/

/**
 * Pop item at top of the linked list. This removes the item.
 * Will also reset the list internal pointer to first item.
 *
 * @param list Linked list.
 * @param len Pointer where to store length of data, or NULL.
 * @return Item data. Must be freed after usage.
 *         Returns NULL, if the list is empty.
 */
void *var_ll_pop(linkedl_t list, size_t *len);

/**
 * Reset linked list current item to first.
 *
 * @param list Linked list.
 */
void var_ll_reset(linkedl_t list);
void *var_ll_each(linkedl_t list, size_t *size);

/**
 * Get count of items in list.
 */
int var_ll_count(linkedl_t list);

void var_ll_free(linkedl_t list);

void var_ll_dump(linkedl_t list);


#endif /* _STRLLIST_H */
/******************************************************************************/

