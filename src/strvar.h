/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRVAR_H
#define _STRVAR_H


/******************************************************************************/
/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


/******************************************************************************/
/* DEFINES */
#define VAR_DEFAULT		0
#define VAR_ALL			-1

#define VAR_LIST_SIZE	(sizeof(struct var_list))
#define VAR_ITEM_SIZE	(sizeof(struct var_item))

#define VAR_TYPE_EMPTY	0
#define VAR_TYPE_STR	1
#define VAR_TYPE_NUM	2
#define VAR_TYPE_BIN	3
#define VAR_TYPE_P		4
#define VAR_TYPE_INT	5

#define VAR_CHARS		"$"
#define VAR_BREAK		" .+$\n"
#define VAR_SPECIAL		"$"

#define VAR_MIN_MALLOC	MAX_STRING

enum
{
	/* whether to expand variables in variables */
	VAR_OPT_EXPAND,
	
	VAR_OPT_C /* must be last */
};

/******************************************************************************/
/* STRUCTS */
/**
 * @addtogroup strvar
 * @{
 */
struct var_item
{
	char key[260];
	int type;
	void *data;
	size_t size;
	struct var_item *next;
	struct var_item *prev;
};
struct var_list
{
	char name[260];
	struct var_item *first;
	struct var_item *last;
	struct var_item *current;
	size_t count;
	int auto_array_counter;
};
typedef int var_list_t;
/** @} addtogroup strvar */


/******************************************************************************/
/* FUNCTION DEFINITIONS */
/**
 * @addtogroup strvar
 * @{
 */
int var_init(void);
void var_quit(void);
void var_dump(void);
#define var_free(p) free(p)
var_list_t varl_new(char *name);
var_list_t varl_find(char *name);
/**
 * Rename existing list.
 *
 * @return Returns 0 on success, -1 on errors
 */
int varl_rename(var_list_t list, char *name);

int varl_set_str(var_list_t, char *, char *, ...);
int varl_set_num(var_list_t, char *, double);
int varl_set_int(var_list_t, char *, int);

/**
 * As varl_set_str, but set variable as binary data with given size.
 *
 * @param list ID of list to be used.
 * @param name Name of item to be set.
 * @param data Pointer to data to be copied into variable.
 * @param size Size of data.
 * @return Returns 0 on success, -1 on errors.
 */
int varl_set_bin(var_list_t list, const char *name, void *data, size_t size);

/**
 * Remove variable. Free all memory reserved by given variable.
 *
 * @param list ID of list to be used.
 * @param name Name of item.
 */
void varl_rm(var_list_t list, const char *name);

const char *varl_get_str(var_list_t, char *);
double varl_get_num(var_list_t, char *);
int varl_get_int(var_list_t, char *);
const void *varl_get_bin(var_list_t, char *, int *);

int varl_is_str(var_list_t, char *, char *);
int varl_is_num(var_list_t, char *, double);
int varl_is_int(var_list_t, char *, int);
//int varl_is_bin(var_list_t, char *, void *, int);
int varl_is_empty(var_list_t, char *);
int varl_is(var_list_t list, char *key);
void varl_true(var_list_t list, char *key);
void varl_false(var_list_t list, char *key);

int varl_strlen(var_list_t, char *);

const char *varl_parsev(var_list_t, char *, int *, int);
const char *varl_parse(var_list_t, char *, void *, ...);
double varl_calc(var_list_t, char *, void *, ...);

/**
 * Parse formatted file into string variables.
 * Format is as follows:
 * <br>01| name=value
 * <br>02| [group1]
 * <br>03| width=640
 * <br>04| height=480
 * <br>05| driver = nvidia framebuffer
 * <br>06| name = "7900GTX"
 * <br>07| [audio]
 * <br>08| volume = 100
 * <br>
 * Previous will cause variable named "name" to be added to default list
 * with value "value". Then list named "group1" is created and lines after
 * that are inserted to that list until line 7. On line 7, new list called
 * "audio" is created and lines after that inserted again into that list.
 * White-spaces (space, tab,..) are always stripped from both ends of line
 * and after equals sign before variable value starts.
 * Name of new list can be in form "[name" or "[name]".
 * But if you insert spaces in the name, they will be included, not removed.
 * Example "[  name  ]" will not be interpreted as "name" but as "  name  ".
 *
 * @param file Filename.
 * @param lists Set to NULL, do not use. Reserved for future.
 * @return 0 on success, -1 on errors.
 */
int varl_file(const char *, void *);

/**
 * Parse formatted file into string variables.
 * Format is as follows:
 * <br>01| name=value
 * <br>02| [group1]
 * <br>03| width=640
 * <br>04| height=480
 * <br>05| driver = nvidia framebuffer
 * <br>06| name = "7900GTX"
 * <br>07| [audio]
 * <br>08| volume = 100
 * <br>
 * Previous will cause variable named "name" to be added to default list
 * with value "value". Then list named "group1" is created and lines after
 * that are inserted to that list until line 7. On line 7, new list called
 * "audio" is created and lines after that inserted again into that list.
 * White-spaces (space, tab,..) are always stripped from both ends of line
 * and after equals sign before variable value starts.
 * Name of new list is very strict. It can be in form "[name" or "[name]".
 * But if you insert spaces in the name, they will be included, not removed.
 * Example "[  name  ]" will not be interpreted as "name" but as "  name  ".
 *
 * @param file Filename.
 * @param lists Set to NULL, do not use. Reserved for future.
 * @param prefunc Function to use filtering variables before adding them to list, or NULL.
 * @return 0 on success, -1 on errors.
 */
int varl_file_prefunc(const char *, void *, int (*prefunc)(var_list_t, const char *, const char *));

void varl_cp(const char *, const char *);

/**
 * Return count of items in list.
 */
int varl_count(var_list_t);

/**
 * Reset list current item to first.
 *
 * @param list List.
 */
void varl_reset(var_list_t list);

/**
 * Go trough all items in list.
 *
 * @param list List.
 * @param type Pointer where to store item type (int).
 * @param value Pointer to pointer where to store pointer to item value (must be freed after use).
 * @param size Pointer where to store item size (size_t).
 * @return Pointer to key of current item (must be freed after use), NULL when no more items are available.
 */
char *varl_each(var_list_t list, int *type, void **value, size_t *size);

/**
 * Get all keys in list.
 */
char **varl_get_keys(var_list_t);

/**
 * Get all keys and data in list.
 */
char **varl_get_all(var_list_t);

#define var_set_str(n, ...) varl_set_str(0, n, __VA_ARGS__)
#define var_set_num(n, v) varl_set_num(0, n, v)
#define var_set_int(n, v) varl_set_int(0, n, v)
#define var_set_bin(n, p, s) varl_set_bin(0, n, p, s)

#define var_rm(n, p) varl_remove(0, n, p)

#define var_get_str(n) varl_get_str(0, n)
#define var_get_num(n) varl_get_num(0, n)
#define var_get_int(n) varl_get_int(0, n)
#define var_get_bin(n, s) varl_get_bin(0, n, s)

#define var_is_str(s, v) varl_is_str(0, s, v)
#define var_is_num(s, v) varl_is_num(0, s, v)
#define var_is_int(s, v) varl_is_int(0, s, v)
#define var_is_empty(s) varl_is_empty(0, s)
#define var_is(s) varl_is(0, s)
#define var_true(s) varl_true(0, s)
#define var_false(s) varl_false(0, s)

#define var_strlen(s) varl_strlen(0, s)

#define var_calc(n) varl_calc(0, n, 0, -1)
#define var_file(f) varl_file(f, NULL)

#define var_count() varl_count(0)

#define var_reset() varl_reset(0)
#define var_each(t, v, s) varl_each(0, t, v, s)

#define var_get_keys() varl_keys(0)

//inline int var_set_(char *, ) { return varl_set_(0, char *, ); }

/** @} addtogroup strvar */


/**
 * @addtogroup internal
 * @{
 */
void _v_free(struct var_item *);
void _v_set(struct var_item *, void *, int, int);
void _v_new(struct var_item **, var_list_t, char *);
struct var_item *_v_find(var_list_t, char *);
int _v_list_set(var_list_t, const char *, void *, int, int);
void _v_strcat(char **, int *, const char *, int);
char *_v_parse(struct var_item *, struct var_list *, int *, int);
/** @} addtogroup internal */


#endif /* _STRVAR_H */
/******************************************************************************/

