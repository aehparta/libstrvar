/*
 * Part of libstrvar.
 *  Simple list: create an array list which is
 *  re-allocated each time new item is added
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include "strslist.h"


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
/**
 * Initialize new simple list.
 *
 * @param sl Pointer to uninitialized simple list struct.
 * @return 0 on success, -1 on errors.
 */
int var_sl_init(struct var_sl *sl)
{
	memset(sl, 0, sizeof(*sl));
	
	if (lock_init(&sl->lock))
	{
		return -1;
	}

	return 0;
}


/******************************************************************************/
/**
 * Add item to list.
 *
 * @param sl Pointer to simple list struct.
 * @param key Not in use, can be NULL always.
 * @param string printf-style formatted string with following arguments.
 * @return New item index on success, -1 on errors.
 */
int var_sl_add(struct var_sl *sl, const char *key, const char *string, ...)
{
	size_t size;
	va_list args;
	char *newstr = NULL;

	lock_write(&sl->lock);
	
	/* Format given ascii data. */
	va_start(args, string);
	size = vasprintf(&newstr, string, args);
	va_end(args);
	if (size < 0)
	{
		lock_unlock(&sl->lock);
		return -1;
	}
	size++; /* Include terminating null char in size. */

	/* Increase size of the list */
	sl->l = (char **)realloc(sl->l, (sl->c + 2) * sizeof(*sl->l));
	sl->l[sl->c] = newstr;
	sl->c++;
	sl->l[sl->c] = NULL;

	lock_unlock(&sl->lock);
	
	return sl->c - 1;
}


/******************************************************************************/
/**
 * Delete item from list.
 *
 * @param sl Pointer to simple list struct.
 */
void var_sl_del(struct var_sl *sl, int index)
{
	lock_write(&sl->lock);

	lock_unlock(&sl->lock);
}


/******************************************************************************/
/**
 * Copy another list to end of another.
 *
 * @param dst Destination list.
 * @param src List where from to copy.
 */
void var_sl_cpe(struct var_sl *dst, struct var_sl *src)
{
	int i, c;
	
	lock_read(&src->lock);
	c = src->c;
	for (i = 0; i < c; i++)
	{
		var_sl_add(dst, /*! \todo key */ NULL, "%s", src->l[i]);
	}
	lock_unlock(&src->lock);
}


/******************************************************************************/
/**
 * Get count of items in list.
 *
 * @param sl Pointer to simple list struct.
 */
int var_sl_c(struct var_sl *sl)
{
	return sl->c;
}


/******************************************************************************/
/**
 * Get direct pointer to list data. This is not thread safe.
 */
char **var_sl_data(struct var_sl *sl)
{
	return sl->l;
}


/******************************************************************************/
/**
 * Free resources reserved for list.
 *
 * @param sl Pointer to simple list struct.
 */
void var_sl_free(struct var_sl *sl)
{
	int i;
	
	lock_write(&sl->lock);
	for (i = 0; i < sl->c; i++)
	{
		free(sl->l[i]);
	}
	free(sl->l);
	sl->c = 0;
	lock_destroy(&sl->lock);
}


/******************************************************************************/
void var_sl_dump(FILE *f, struct var_sl *sl)
{
	int i;
	
	lock_write(&sl->lock);
	for (i = 0; i < sl->c; i++)
	{
		fprintf(f, "%s\n", sl->l[i]);
	}
	lock_unlock(&sl->lock);
}


/******************************************************************************/
char *var_sl_at(struct var_sl *sl, int index)
{
	char *found = NULL;
	lock_read(&sl->lock);
	if (sl->c > index)
	{
		found = strdup(sl->l[index]);
	}
	lock_unlock(&sl->lock);
	return found;
}

