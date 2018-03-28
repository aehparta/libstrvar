/*
 * Part of libstrvar.
 *
 * Linked list: Simple linked list.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include "strllist.h"


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
int _var_ll_app(struct var_llist *list, void *data, size_t len, int type)
{
	struct var_llist_item *item;

	item = (struct var_llist_item *)malloc(sizeof(*item));
	if (!item) return -1;
	memset(item, 0, sizeof(*item));
	item->type = type;
	item->size = len;

	if (type == VAR_TYPE_P || type == VAR_TYPE_INT)
	{
		item->data = data;
	}
	else if (type != VAR_TYPE_STR)
	{
		item->data = malloc(len);
		if (!item->data)
		{
			free(item);
			return -1;
		}
		memcpy(item->data, data, len);
	}
	else item->data = data;

	lock_write(&list->lock);
	if (list->first)
	{
		list->last->next = item;
		list->last = item;
	}
	else
	{
		list->first = item;
		list->last = item;
		list->current = item;
	}
	list->count++;
	lock_unlock(&list->lock);

	return 0;
}


/******************************************************************************/
struct var_llist *var_ll_new(void)
{
	struct var_llist *list;
	
	list = (struct var_llist *)malloc(sizeof(*list));
	if (!list) return NULL;
	memset(list, 0, sizeof(*list));

	if (lock_init(&list->lock))
	{
		free(list);
		return NULL;
	}

	return list;
}


/******************************************************************************/
int var_ll_app(struct var_llist *list, const char *string, ...)
{
	struct var_llist_item *item;
	va_list args;
	char *newstr = NULL;
	size_t size;
	
	/* Format given ascii data. */
	va_start(args, string);
	size = vasprintf(&newstr, string, args);
	va_end(args);
	if (size < 0) return;
	size++; /* Include terminating null char in size. */

	return _var_ll_app(list, newstr, size, VAR_TYPE_STR);
}


/******************************************************************************/
int var_ll_app_bin(struct var_llist *list, void *data, size_t len)
{
	return _var_ll_app(list, data, len, VAR_TYPE_BIN);
}


/******************************************************************************/
int var_ll_app_p(linkedl_t list, void *pointer)
{
	return _var_ll_app(list, pointer, 0, VAR_TYPE_P);
}


/******************************************************************************/
int var_ll_app_int(linkedl_t list, int value)
{
	return _var_ll_app(list, (void *)value, 0, VAR_TYPE_INT);
}


/******************************************************************************/
void *var_ll_pop(linkedl_t list, size_t *len)
{
	void *data = NULL;
	struct var_llist_item *item;

	lock_write(&list->lock);
	if (list->count < 1 || !list->first)
	{
		list->count = 0;
		lock_unlock(&list->lock);
		return NULL;
	}

	item = list->first;
	data = item->data;
	if (len) *len = item->size;
	list->first = item->next;
	list->current = list->first;
	list->count--;

	free(item);
	lock_unlock(&list->lock);

	return data;
}


/******************************************************************************/
void var_ll_reset(linkedl_t list)
{
	lock_write(&list->lock);
	list->current = list->first;
	lock_unlock(&list->lock);
}


/******************************************************************************/
/**
 * Get next item in linked list.
 *
 * @param list Linked list.
 * @param size Pointer where to store size of data, or NULL.
 * @return Pointer to item data or NULL when no items.
 */
void *var_ll_each(linkedl_t list, size_t *size)
{
	void *p = NULL;
	
	lock_write(&list->lock);
	if (list->current)
	{
		p = list->current->data;
		if (size) (*size) = list->current->size;
		list->current = list->current->next;
	}
	lock_unlock(&list->lock);
	
	return p;
}


/******************************************************************************/
int var_ll_count(struct var_llist *list)
{
	int err = 0;
	if (!list) return 0;
	lock_read(&list->lock);
	err = list->count;
	lock_unlock(&list->lock);
	return err;
}


/******************************************************************************/
/**
 * Free linked list.
 *
 * @param list Linked list.
 */
void var_ll_free(struct var_llist *list)
{
	struct var_llist_item *i1, *i2;

	if (!list) return;
	lock_write(&list->lock);
	
	for (i1 = list->first; i1; i1 = i2)
	{
		if (i1->data && (i1->type != VAR_TYPE_P && i1->type != VAR_TYPE_INT))
		{
			free(i1->data);
		}
		i2 = i1->next;
		free(i1);
	}
	
	lock_destroy(&list->lock);
	free(list);
}


/******************************************************************************/
/**
 * Dump linked list.
 *
 * @param list Linked list.
 */
void var_ll_dump(struct var_llist *list)
{
	struct var_llist_item *v;
	int i;
	char *type, content[1024];
	
	lock_read(&list->lock);
	
	for (i = 0, v = list->first; v; i++, v = v->next)
	{
		switch (v->type)
		{
		case VAR_TYPE_EMPTY:
			type = "empty";
			break;
		case VAR_TYPE_STR:
			type = "ascii";
			break;
		case VAR_TYPE_NUM:
			type = "number";
			break;
		case VAR_TYPE_BIN:
			type = "binary";
			break;
		case VAR_TYPE_P:
			type = "pointer";
		case VAR_TYPE_INT:
			type = "integer";
			break;
		}
		
		memset(content, 0, sizeof(content));
		if (v->type == VAR_TYPE_P) sprintf(content, "%p", v->data);
		else if (v->type == VAR_TYPE_INT) sprintf(content, "%d", (int)v->data);
		else if (v->type == VAR_TYPE_STR)
		{
			strncpy(content, v->data, 30);
			if (strlen(v->data) > strlen(content)) strcat(content, "...");
		}
		else if (v->type == VAR_TYPE_NUM) sprintf(content, "%lf", *((double *)v->data));
		printf("%d. type \'%s\', size %d, content \'%s\'\n", i, type, (int)v->size, content);
	}
	
	lock_unlock(&list->lock);
}

