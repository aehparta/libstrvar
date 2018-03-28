/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRHASH_H
#define _STRHASH_H


#ifdef __cplusplus
extern "C" {
#endif

#include <ddebug/synchro.h>
#include <ddebug/debuglib.h>
#include "strvar.h"


/******************************************************************************/
#define HASHL_DEFAULT_SIZE		32

/* some defines for internal use */
#define HASH_DOGET				0
#define HASH_DOPUT				1
#define HASH_DOINC				2
#define HASH_GETITEM			3
#define HASH_DORM				4
#define HASH_DOPOP				5


/******************************************************************************/
struct var_hashlist
{
	struct var_item **items;
	size_t size;
	
	unsigned long (*f_hash)(int, void *);
	int (*f_keylen)(void *);

	void (*f_free)(struct var_hashlist *, const void *, void *);
	
	lock_t lock;

	struct var_item *current_item;
	int current_hash;
};
typedef struct var_hashlist * hashl_t;
typedef struct var_hashlist * var_lh_t;


/******************************************************************************/

/**
 * Predefined hash function for uint32 type keys. Max (length for var_lh_new())
 * must be multiple of 2!
 */
unsigned long var_lh_uint32_hash(int max, void *key);
/**
 * Predefined length function for uint32 type keys, use with var_lh_uint32_hash.
 */
int var_lh_uint32_len(void *key);

hashl_t var_lh_new(int length, unsigned long (*key_hash)(int, void *), int (*key_len)(void *));
void var_lh_free(hashl_t list);
void *var_lh_puta(hashl_t list, const void *key, const char *string);
void *var_lh_putb(hashl_t list, const void *key, const void *data, size_t size);
void var_lh_setp(hashl_t list, const void *key, void *pointer);
void var_lh_setnum(hashl_t list, const void *key, double value);
void var_lh_addnum(hashl_t list, const void *key, double value);
int var_lh_get(hashl_t list, const void *, void **, size_t *);
void *var_lh_getp(hashl_t list, const void *);
int var_lh_get_int(hashl_t list, const void *);
int var_lh_count(hashl_t list);
void var_lh_dump(hashl_t list);


/**
 * Reset list to first item so that var_lh_each() will loop trough all.
 */
void var_lh_reset(hashl_t list);


/**
 * Return next item in list.
 *
 * @param list List to be used.
 * @param key Pointer to pointer where to save key pointer :P
 * @param size Pointer to size_t variable where to save item size, or NULL.
 * @return Pointer to item.
 */
void *var_lh_each(hashl_t list, void **key, size_t *size);


/**
 * Loop trough list and call funtion for each item.
 *
 * @param list List to be used.
 * @param function Function to call for each item. Function must
 *                 return 0 to continue the foreach, anything else
 *                 will exit the loop.
 */
void var_lh_foreach(hashl_t list,
                    int (*function)(hashl_t list,
                                    const char *key,
                                    void *data,
                                    size_t size,
                                    int type,
                                    unsigned long hash,
                                    void *user),
                    void *user);
void var_lh_clear(hashl_t list);

void var_lh_func_rm(hashl_t list, void (*free)(var_lh_t list, const void *key, void *pointer));
int var_lh_rm(hashl_t list, const void *key);

void *var_lh_popp(hashl_t list);

unsigned long var_lh_default_hash(int max, unsigned char *str);


#ifdef __cplusplus
}
#endif

#endif /* _STRHASH_H */
/******************************************************************************/

