/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include <stdlib.h>
#include <string.h>
#include <ddebug/strlens.h>
#include "strhash.h"
#include "strvar.h"


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
unsigned long var_lh_uint32_hash(int max, void *key)
{
	uint32_t key_uint = *((int *)key);
	return (unsigned long)((max - 1) & key_uint);
}


/******************************************************************************/
int var_lh_uint32_len(void *key)
{
	return sizeof(uint32_t);
}


/******************************************************************************/
/**
 * Internal help routine: Make hash from string.
 */
static inline unsigned long _hl_hash(int max, unsigned char *str)
{
	unsigned long hash = 0;
	int c;

	while (c = *str++) hash = c + (hash << 6) + (hash << 16) - hash;
	
	if (max < 0x100)
	{
		hash = (hash & 0xff) ^ ((hash >> 8) & 0xff) ^
		       ((hash >> 16) & 0xff) ^ ((hash >> 24) & 0xff);
	}
	else if (max < 0x10000) hash = (hash & 0xffff) ^ (hash >> 16);
	
	hash = hash % max;
	
	return hash;
}


/******************************************************************************/
/**
 * Internal help routine: Copy data from item to another.
 */
static inline void *_hl_item_data_copy(struct var_item *dst, struct var_item *src)
{
	if (src->size > dst->size)
	{
		if (dst->data) free(dst->data);
		dst->data = malloc(src->size);
		dst->size = 0;
		if (!dst->data) return;
	}
	dst->size = src->size;
	dst->type = src->type;
	memcpy(dst->data, src->data, dst->size);

	return dst->data;
}


/******************************************************************************/
/**
 * Internal help routine: Find/add from/to hashlist.
 *
 * @return Non-zero if found.
 */
int _hl_find(struct var_hashlist *list, struct var_item *item, int _do, void **datapr)
{
	struct var_item *loop, *from = NULL, *to = NULL;
	unsigned long hash;
	int err = 0;
	void *datap = NULL;
	
	lock_write(&list->lock);

	if (_do == HASH_DOPOP)
	{
		int i;
		err = 0;
		
		for (i = 0; i < list->size; i++)
		{
			if (list->items[i] != NULL)
			{
				if (list->items[i]->type != VAR_TYPE_P) continue;
				
				struct var_item *h, *from = list->items[i];
				void *p = *((void **)from->data);
				
				h = from->prev;
				if (h) h->next = from->next;
				h = from->next;
				if (h) h->prev = from->prev;
				if (list->items[i] == from && from->next == NULL) list->items[i] = NULL;
				else if (list->items[i] == from)
				{
					list->items[i] = from->next;
					list->items[i]->prev = NULL;
				}
				*datapr = from->data;
				free(from);
				err = 1;
				break;
			}
		}
		
		lock_unlock(&list->lock);
		return err;
	}

	/* calculate hash first. */
	hash = list->f_hash(list->size, item->key);
	
	for (loop = list->items[hash]; loop; loop = loop->next)
	{
		if (list->f_keylen(loop->key) != list->f_keylen(item->key));
		else if (memcmp(loop->key, item->key, list->f_keylen(item->key)) == 0)
		{
			switch (_do)
			{
			default:
			case HASH_DOGET:
				from = loop;
				to = item;
				break;
				
			case HASH_DOPUT:
				from = item;
				to = loop;
				break;
				
			case HASH_DOINC:
				if (loop->type == VAR_TYPE_NUM)
				{
					double *value = (double *)loop->data;
					double *add = (double *)item->data;
					(*value) += (*add);
				}
				err = 1;
				goto out_err;

			case HASH_GETITEM:
				item->data = loop->data;
				item->size = loop->size;
				err = 1;
				goto out_err;

			case HASH_DOPOP:
			case HASH_DORM:
				from = loop;
				{
					struct var_item *h;
					void *p = *((void **)from->data);
					if (list->f_free && _do != HASH_DOPOP) list->f_free(list, from->key, p);
					h = from->prev;
					if (h) h->next = from->next;
					h = from->next;
					if (h) h->prev = from->prev;
					if (list->items[hash] == from && from->next == NULL) list->items[hash] = NULL;
					else if (list->items[hash] == from)
					{
						list->items[hash] = from->next;
						list->items[hash]->prev = NULL;
					}
					free(from->data);
					free(from);
				}
				err = 1;
				goto out_err;
			}
			
			datap = _hl_item_data_copy(to, from);
			err = 1;
			goto out_err;
		}
		
		if (!loop->next) break;
		/* this is here because when we do add(/etc),
		 * loop must point to last item in this hashes
		 * items so that add works later.
		 */
	}

	/* do add of new item (loop should not be null if possible ) */
	if (_do == HASH_DOPUT || _do == HASH_DOINC)
	{
		to = (struct var_item *)malloc(sizeof(*to));
		IF_ER(!to, 0);
		memset(to, 0, sizeof(*to));
		memcpy(to->key, item->key, list->f_keylen(item->key));
		datap = _hl_item_data_copy(to, item);
	
		if (!list->items[hash]) list->items[hash] = to;
		else if (loop)
		{
			loop->next = to;
			to->prev = loop;
		}
	}

out_err:
	lock_unlock(&list->lock);
	if (datapr) *datapr = datap;
	return err;
}


/******************************************************************************/
/**
 * Internal help routine: Put new data into hashlist.
 */
void *_hl_putb(struct var_hashlist *list, const void *key, const void *data, int size, int type)
{
	struct var_item item;
	void *datap = NULL;

	/* Setup new hashlist item. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	item.data = (void *)data;
	item.size = size;
	item.type = type;

	/* Add new item to list. */
	
	_hl_find(list, &item, HASH_DOPUT, &datap);
	return datap;
}


/******************************************************************************/
/**
 * Internal help routine: Set/add num into hashlist.
 */
void _hl_putn(struct var_hashlist *list, const void *key, double value, int _do)
{
	struct var_item item;
	
	/* Setup new hashlist item. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	item.data = &value;
	item.size = sizeof(value);
	item.type = VAR_TYPE_NUM;

	/* Add new item to list. */
	_hl_find(list, &item, _do, NULL);
}


/******************************************************************************/
/**
 * Initialize hashlist.
 *
 * @param size Size of hashlist, or 0 for default (HASHL_DEFAULT_SIZE).
 * @param hash Function to make hash of key. Can be NULL.
 *             Default hash function presumes that key is a string.
 * @param len Function to retrieve size of key. Can be NULL.
 *            Default function is strlen().
 * @return Pointer to new hashlist or NULL on errors.
 */
struct var_hashlist *var_lh_new(int size, unsigned long (*hash)(int, void *), int (*len)(void *))
{
	struct var_hashlist *list = NULL;
	
	/* Reset to default size, if needed. */
	if (size < 1) size = HASHL_DEFAULT_SIZE;
	
	/* Allocate new list. */
	list = (struct var_hashlist *)malloc(sizeof(*list));
	if (!list) return NULL;
	memset(list, 0, sizeof(*list));
	list->items = (struct var_item **)malloc(sizeof(*list->items) * size);
	if (!list->items)
	{
		free(list);
		return NULL;
	}
	memset(list->items, 0, sizeof(*list->items) * size);
	list->size = size;
	if (!hash) list->f_hash = (unsigned long (*)(int, void *))_hl_hash;
	else list->f_hash = hash;
	if (!len) list->f_keylen = (int (*)(void *))strlen;
	else list->f_keylen = len;
	
	lock_init(&list->lock);
	
	return list;
}


/******************************************************************************/
/**
 * Clear hashlist, make the list empty.
 *
 * @param list List to be cleared.
 */
void var_lh_clear(struct var_hashlist *list)
{
	struct var_item *v1, *v2;
	int i;

	lock_write(&list->lock);
	if (list->items)
	{
		for (i = 0; i < list->size; i++)
		{
			for (v1 = list->items[i]; v1; v1 = v2)
			{
				v2 = v1->next;
				if (list->f_free && v1->type == VAR_TYPE_P)
				{
					list->f_free(list, v1->key, *((void **)v1->data));
				}
				if (v1->data) free(v1->data);
				free(v1);
			}
			list->items[i] = NULL;
		}
	}
	lock_unlock(&list->lock);
}


/******************************************************************************/
/**
 * Free hashlist.
 *
 * @param list List to be freed.
 */
void var_lh_free(struct var_hashlist *list)
{
	struct var_item *v1, *v2;
	int i;
	
	var_lh_clear(list);
	free(list->items);
	lock_destroy(&list->lock);
	free(list);
}


/******************************************************************************/
/**
 * Put new ascii string into hashlist.
 *
 * @param list List to be used.
 * @param key Key to be used.
 * @param str String to be inserted/replaced for given key.
 */
void *var_lh_puta(struct var_hashlist *list, const void *key, const char *str)
{
	return _hl_putb(list, key, str, strlen(str) + 1, VAR_TYPE_STR);
}


/******************************************************************************/
/**
 * Put new data into hashlist.
 *
 * @param list List to be used.
 * @param key Key to be used.
 * @param data Data to be inserted/replaced for given key.
 * @param size Size of data.
 */
void *var_lh_putb(struct var_hashlist *list, const void *key, const void *data, size_t size)
{
	return _hl_putb(list, key, data, size, VAR_TYPE_BIN);
}


/******************************************************************************/
/**
 * Set pointer value as hashlist item.
 *
 * @param list List to be used.
 * @param key Key to be used.
 * @param pointer Pointer to set.
 */
void var_lh_setp(struct var_hashlist *list, const void *key, void *pointer)
{
	_hl_putb(list, key, &pointer, sizeof(pointer), VAR_TYPE_P);
}


/******************************************************************************/
/**
 * Set number value to hashlist.
 *
 * @param list List to be used.
 * @param key Key to be used.
 * @param value Value to be set for the key.
 */
void var_lh_setnum(struct var_hashlist *list, const void *key, double value)
{
	_hl_putn(list, key, value, HASH_DOPUT);
}


/******************************************************************************/
/**
 * Add value to number in hashlist (or set value, if key not found).
 *
 * @param list List to be used.
 * @param key Key to be used.
 * @param value Value to be add/set for the key.
 */
void var_lh_addnum(struct var_hashlist *list, const void *key, double value)
{
	_hl_putn(list, key, value, HASH_DOINC);
}


/******************************************************************************/
/**
 * Get data pointer from hashlist.
 */
void *var_lh_getp(struct var_hashlist *list, const void *key)
{
	struct var_item item;

	/* Setup hashlist item for search. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	item.data = NULL;
	item.size = 0;
	
	/* Find item. */
	if (_hl_find(list, &item, HASH_GETITEM, NULL))
	{
		return *((void **)item.data);
	}

	return NULL;
}


/******************************************************************************/
/**
 * Get data from hashlist and return value as integer (zero on errors).
 */
int var_lh_get_int(struct var_hashlist *list, const void *key)
{
	struct var_item item;

	/* Setup hashlist item for search. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	item.data = NULL;
	item.size = 0;

	/* Find item. */
	if (_hl_find(list, &item, HASH_GETITEM, NULL))
	{
		return (int)atoi(item.data);
	}

	return 0;
}


/******************************************************************************/
/**
 * Get data from hashlist.
 *
 * @param list List to be used.
 * @param key Key to use for search.
 * @param data *data should be pointer where to store data. If *data is NULL
 *             Then this function will allocate space needed.
 *             If *data is not NULL, *size must tell the size of previously
 *             malloc():ed space in *data. If data returned by this function
 *             is larger than size of *data buffer, then the space will be
 *             reallocated and new size set to *size.
 * @param size Size of *data, can be NULL always.
 * @return Greater or equal to 0 if data with key found, -1 if not.
 */
int var_lh_get(struct var_hashlist *list, const void *key, void **data, size_t *size)
{
	struct var_item item;

	/* Setup hashlist item for search. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	if (size && *data) 
	{
		item.data = *data;
		item.size = *size;
	}
	else
	{
		if (*data) free(*data);
		item.data = NULL;
		item.size = 0;
	}

	/* Find item. */
	if (_hl_find(list, &item, HASH_DOGET, NULL))
	{
		*data = item.data;
		if (size) *size = item.size;
		return item.type;
	}
	
/*	if (*data)
	{
		free(*data);
		*data = NULL;
	}*/
// 	if (size) *size = 0;

	return -1;
}


/******************************************************************************/
/**
 * Count items in hashlist.
 *
 * @param list List to be used.
 * @return Number of items in list.
 */
int var_lh_count(struct var_hashlist *list)
{
	struct var_item *v;
	int i, n;
	
	lock_read(&list->lock);
	for (i = 0, n = 0; i < list->size; i++)
	{
		for (v = list->items[i]; v; v = v->next, n++);
	}
	lock_unlock(&list->lock);

	return n;
}


/******************************************************************************/
/** Dump debug info of given hashlist. */
void var_lh_dump(struct var_hashlist *list)
{
	struct var_item *v;
	int i, j;
	char *type, content[MAX_STRING], key[MAX_STRING];
	
	printf("** printing items in hashlist (size %d)\n", (int)list->size);
	
	lock_read(&list->lock);
	for (i = 0; i < list->size; i++)
	{
		printf(" * items for hash %d:\n", i);
		for (v = list->items[i], j = 0; v; v = v->next, j++)
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
				break;
			}
			
			memset(content, 0, sizeof(content));
			if (v->type == VAR_TYPE_STR)
			{
				strncpy(content, v->data, 20);
				if (strlen(v->data) > strlen(content)) strcat(content, "...");
			}
			if (v->type == VAR_TYPE_NUM)
			{
				sprintf(content, "%lf", *((double *)v->data));
			}
			
			if (v->type == VAR_TYPE_P)
			{
				sprintf(content, "%p", *((void **)v->data));
			}

			if (list->f_hash == var_lh_uint32_hash)
			{
				sprintf(key, "%d", *((int *) v->key));
			}
			else strcpy(key, v->key);
			printf("   %d. key \'%s\', type \'%s\', size %d, content \'%s\'\n", j, key, type, (int)v->size, content);
		}
	}
	lock_unlock(&list->lock);
	printf("** end of hashlist\n");
}


/******************************************************************************/
void var_lh_reset(hashl_t list)
{
	list->current_item = NULL;
	list->current_hash = 0;
}


/******************************************************************************/
void *var_lh_each(hashl_t list, void **key, size_t *size)
{
	struct var_item *next = list->current_item;
	int hash = list->current_hash;
	
	do
	{
		if (next == NULL)
		{
			next = list->items[hash];
			if (next == NULL)
			{
				hash++;
				if (hash >= list->size) return NULL;
			}
		}
		else
		{
			next = next->next;
			if (next == NULL)
			{
				hash++;
				if (hash >= list->size) return NULL;
			}
		}
	}
	while (!next);

	list->current_item = next;
	list->current_hash = hash;
	
	if (size) *size = next->size;
	return next->data;
}


/******************************************************************************/
void var_lh_foreach(hashl_t list,
                    int (*function)(hashl_t list,
                                    const char *key,
                                    void *data,
                                    size_t size,
                                    int type,
                                    unsigned long hash,
                                    void *user),
                    void *user)
{
	struct var_item *v;
	unsigned long i;
	size_t j;
	int err = 0;
	
	lock_read(&list->lock);
	for (i = 0; (i < list->size) && !err; i++)
	{
		for (v = list->items[i], j = 0; v && !err; v = v->next, j++)
		{
			err = function(list, v->key, v->data, v->size, v->type, i, user);
			if (err != 0) break;
		}
		if (err != 0) break;
	}
	lock_unlock(&list->lock);
}


/******************************************************************************/
/**
 * Create hash from string using hashlist default hash function.
 */
unsigned long var_lh_default_hash(int max, unsigned char *str)
{
	return _hl_hash(max, str);
}


/******************************************************************************/
void var_lh_func_rm(hashl_t list, void (*free)(var_lh_t list, const void *key, void *pointer))
{
	list->f_free = free;
}


/******************************************************************************/
int var_lh_rm(hashl_t list, const void *key)
{
	struct var_item item;

	/* Setup hashlist item for search. */
	memset(item.key, 0, sizeof(item.key));
	memcpy(item.key, key, list->f_keylen((void *)key));
	item.data = NULL;
	item.size = 0;

	/* Find item. */
	return _hl_find(list, &item, HASH_DORM, NULL);
}


/******************************************************************************/
void *var_lh_popp(hashl_t list)
{
	void *data = NULL, *p = NULL;

	/* Find item. */
	if (_hl_find(list, NULL, HASH_DOPOP, &data))
	{
		p = *((void **)data);
		free(data);
	}

	return p;
}

