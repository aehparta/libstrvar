/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include <ddebug/synchro.h>
#include "strvar.h"
#include <ddebug/strlens.h>
#include "strcalc.h"


#ifdef _DEBUG
#define PRINTF printf
#else
#define PRINTF
#endif


/******************************************************************************/
/* VARIABLES */
/* Variable array container. */
static struct var_list *var_list = NULL;
/* Variable array size. */
static int var_list_c = 0;
/* Lock for variable array. */
static lock_t var_list_lock;
/* Constant empty variable string for internal use. */
static char *var_empty_string = "";
/* settings */
static void *vopt[VAR_OPT_C] =
{
	0,
};


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
/**
 * Internal help routine:
 * Strip white-spaces from the beginning and end of a string.
 * This function returns pointer to first non white-space character and sets
 * null character at first non white-space looking from end of the string.
 * If string contains only white-spaces, pointer returned is set at terminating
 * null character of the string.
 *
 * @param str String to be handled.
 * @return Pointer to first non white-space character.
 */
char *_v_trim(char *str)
{
	char *p;

	/* Find first non white-space. */
	for ( ; isspace((int)(*str)); str++);
	/* Check if the string contains only white-spaces. */
	if (*str == '\0') return str;

	/* Find last non white-space. */
	for (p = str + strlen(str) - 1; isspace((int)(*p)); p--);
	p[1] = '\0';
	
	return str;
}


/******************************************************************************/
/**
 * Internal help routine:
 * First strips white-spaces from the beginning and end of a string.
 * Then reduces all sequential white-spaces to one for each sequence.
 *
 * @param str String to be handled.
 * @return Pointer to first non white-space character.
 */
char *_v_trim_all(char *str)
{
	char *p, *p2;
	int len, i, space = 0;

	/* First trim start and end. */
	str = _v_trim(str);

	p = p2 = str;
	len = strlen(str);

	for (i = 0; i < len; i++)
	{
		if (isspace((int)(*p)))
		{
			if (!space)
			{
				*p2 = ' ';
				p2++;
			}
			p++;
			space = 1;
		}
		else
		{
			*p2 = *p;
			p2++;
			p++;
			space = 0;
		}
	}
	
	*p2 = '\0';
	
	return str;
}

/******************************************************************************/
/**
 * Internal help routine: Free variable item.
 * @note Wont lock var_list.
 */
void _v_free(struct var_item *v)
{
	if (v)
	{
		if (v->data)
		{
			free(v->data);
			v->data = NULL;
			v->size = 0;
			v->type = VAR_TYPE_EMPTY;
		}
	}
}


/******************************************************************************/
/**
 * Internal help routine: Set (and allocate) new data for item.
 * @note Wont lock var_list.
 */
void _v_set(struct var_item *v, void *data, int size, int type)
{
	/* If more buffer needed, allocate it and free old. */
	if (v->size < size)
	{
		_v_free(v);
		v->data = malloc(size);
		if (v->data) v->size = size;
		else v->size = 0;
	}
	/* Copy new data and set type. */
	v->type = type;
	if (v->data) memcpy(v->data, data, size);
}


/******************************************************************************/
/**
 * Internal help routine: Allocate new item in list.
 * @note Wont lock var_list.
 */
void _v_new(struct var_item **v, var_list_t list, char *name)
{
	*v = (struct var_item *)malloc(VAR_ITEM_SIZE);
	if (*v)
	{
		memset(*v, 0, VAR_ITEM_SIZE);
		if (name) STRCPY((*v)->key, name);
		(*v)->type = VAR_TYPE_EMPTY;
		if (!var_list[list].first)
		{
			var_list[list].first = *v;
			var_list[list].last = *v;
		}
		else
		{
			var_list[list].last->next = *v;
			var_list[list].last = *v;
		}
		var_list[list].count++;
	}
}


/******************************************************************************/
/**
 * Internal help routine: Find variable.
 * @note Wont lock var_list.
 *
 * @param list List ID which to used in search.
 * @param name Name of variable to find.
 * @return Pointer to variable struct, or NULL.
 */
struct var_item *_v_find(var_list_t list, char *name)
{
	struct var_item *v;
	int i, n;
	
	/* Return error, if lib not initialized yet. */
	if (!var_list) return NULL;
	
	/* Check search conditions. */
	if (list < 0)
	{
		i = 0;
		n = var_list_c;
	}
	else if (list < var_list_c)
	{
		i = list;
		n = list + 1;
	}
	else return NULL;

	for ( ; i < n; i++)
	{
		for (v = var_list[i].first; v; v = (struct var_item *)v->next)
		{
			if (strcmp(v->key, name) == 0) return v;
		}
	}

	return NULL;
}


/******************************************************************************/
/**
 * Internal help routine: Set variable data to given.
 * @note Lock's var_list when needed.
 *
 * @param list ID of list to be used.
 * @param name Name of item to be set.
 * @param data Pointer to data to be set.
 * @param size Size of data.
 * @param type Type of data to be set.
 * @return Returns 0 on success, -1 on errors.
 */
int _v_list_set(var_list_t list, const char *name, void *data, int size, int type)
{
	int i, n, create, err;
	struct var_item *v;
	char *name_real = (char *)name;
	
	/* Return error, if lib not initialized yet. */
	if (!var_list) return -1;
	/* Return error, if name is invalid. */
	if (!name) return -1;

	lock_write(&var_list_lock);

	/* Check search conditions. */
	if (list < 0)
	{
		i = 0;
		n = var_list_c;
		create = 0;
	}
	else if (list < var_list_c)
	{
		i = list;
		n = list + 1;
		create = 1;
	}
	else
	{
		err = -1;
		goto out_err;
	}

	/* if name is null, autogenerate it */
	while (!name && list > 0 && list < var_list_c)
	{
		asprintf(&name_real, "%d", var_list[list].auto_array_counter);
		v = _v_find(list, name_real);
		var_list[list].auto_array_counter++;
		if (!v) break;
	}

	/* Go trough requested item(s). */
	for ( ; i < n; i++)
	{
		/* Try to find item from list. */
		v = _v_find(i, name_real);

		/* Create new item, if needed. */
		if (!v && create) _v_new(&v, i, name_real);
		
		/* Setup new data, if item found/created. */
		if (v) _v_set(v, data, size, type);
	}

	err = 0;

out_err:
	if (!name && name_real) free(name_real);
	lock_unlock(&var_list_lock);
	return err;
}


/******************************************************************************/
/**
 * Internal help routine: Add string after string.
 * @note Wont lock var_list.
 *
 * @param dst Destination string.
 *            Reallocates space here, if needed.
 *            Can be NULL.
 * @param size Size of destination buffer. Stores new size here also.
 * @param src Source string.
 * @param max Maximum characters to be copied, or -1 for no maximum.
 */
void _v_strcat(char **dst, int *size, const char *src, int max)
{
	int n;
	
	/* Just check couple things first for sure. */
	if (!src || max == 0) return;
	if (src[0] == '\0') return;

	/* Get length of source and check if new space is needed. */
	n = strlen(src) + 1; /* +1 includes null char. */
	if (n > *size || !*dst)
	{
		/* Clear these, if either is not valid according to other. */
		if (!*dst) *size = 0;
		else if (*size < 1) *dst = NULL;
		
		/* Allocate VAR_MIN_MALLOC multiples of new space. */
		n = *size + VAR_MIN_MALLOC * (1 + (int)(n / VAR_MIN_MALLOC));
		*dst = realloc(*dst, n);
		if (!*dst) *size = 0;
		else
		{
			memset(&((*dst)[*size]), 0, n - *size);
			*size = n;
		}
	}
	
	/* Append source to destination. */
	if (max > 0) strncat(*dst, src, max);
	else strcat(*dst, src);
}


/******************************************************************************/
/**
 * Internal help routine: Parse variable into buffer.
 * @note Wont lock var_list.
 */
char *_v_parse(struct var_item *v, struct var_list *recursion, int *lists, int lc)
{
	int i, j, n = 0;
	char *result = NULL, *res, *var, *varprev, *subvar;
	struct var_item *subv, node, *prevnode = NULL;
	
	/* Check that type is supported in this function. */
	switch (v->type)
	{
	case VAR_TYPE_STR:
	case VAR_TYPE_NUM:
		break;
	default:
		return var_empty_string;
	}

	/*
	 * Check that this node is not already visited previously on
	 * lower level. If this check is not done, we could end up in
	 * dead lock, adding contents of variables inside each other.
	 */
	for (prevnode = recursion->first; prevnode; prevnode = prevnode->next)
	{
		if (prevnode->data == v) return var_empty_string;
	}
	prevnode = NULL;

	/* Add current item to recursion stack. */
	node.data = v;
	node.next = NULL;
	if (!recursion->first)
	{
		recursion->first = &node;
		recursion->last = &node;
	}
	else
	{
		recursion->last->next = &node;
		prevnode = recursion->last;
		recursion->last = &node;
	}
	recursion->count++;

	/*
	 * Parse variable content.
	 * This mostly means converting $<variable name> to their contents.
	 */
	var = (char *)v->data;
	n = 0;
	result = NULL;
	for ( ; ; )
	{
		varprev = var;
		var = strpbrk(var, VAR_CHARS);
		
		if (!var)
		{
			_v_strcat(&result, &n, varprev, -1);
			break;
		}
		
		_v_strcat(&result, &n, varprev, (int)(var - varprev));

		if (strchr(VAR_SPECIAL, (int)var[1]))
		{
			_v_strcat(&result, &n, &var[1], 1);
			var += 2;
			continue;
		}
		
		i = sscanf(&var[1], "%a[^" VAR_BREAK "]", &res);
		if (i == 1)
		{
			var++;
			subv = NULL;
			for (j = 0; j < lc && !subv; j++) subv = _v_find(lists[j], res);
			if (!subv) subvar = var_empty_string;
			else subvar = _v_parse(subv, recursion, lists, lc);
			_v_strcat(&result, &n, subvar, -1);
			var += strlen(res);
			if (strchr(VAR_CHARS, (int)*var)) var++;
			var_free(subvar);
			free(res);
		}
		else var++;
	}

	/* Remove us from the recursion stack. */
	if (recursion->count > 0)
	{
		recursion->last = prevnode;
		if (prevnode) recursion->last->next = NULL;
		recursion->count--;
	}

	/* Return result. */
	if (!result) result = var_empty_string;
	return result;
}


/******************************************************************************/
/**
 * Internal help routine: Find best match for keystr from variables.
 * @note Wont lock var_list.
 *
 * @param list List ID which to used in search.
 * @param keystr String which to try match variables name.
 * @return Pointer to variable struct, or NULL.
 */
struct var_item *_v_find_best(var_list_t list, char *keystr)
{
	struct var_item *v, *vret = NULL;
	int i, n, len;
	
	/* Return error, if lib not initialized yet. */
	if (!var_list) return NULL;
	
	/* Check search conditions. */
	if (list < 0)
	{
		i = 0;
		n = var_list_c;
	}
	else if (list < var_list_c)
	{
		i = list;
		n = list + 1;
	}
	else return NULL;

	for (len = 0; i < n; i++)
	{
		for (v = var_list[i].first; v; v = (struct var_item *)v->next)
		{
			int l = strlen(v->key);
			if (strncmp(v->key, keystr, l) == 0 && l > len)
			{
				vret = v;
			}
		}
	}

	return vret;
}


/******************************************************************************/
/**
 * Initialize variable library.
 * Returns 0 also, if init function already called.
 *
 * @return 0 on success, -1 on errors.
 */
int var_init(void)
{
	/* Dont init, if already init. */
	if (var_list) return 0;

	/* Allocate default list. */
	var_list_c = 0;
	var_list = (struct var_list *)malloc(VAR_LIST_SIZE);
	if (!var_list) return -1;
	memset(&var_list[var_list_c], 0, VAR_LIST_SIZE);
	var_list_c = 1;
	
	if (lock_init(&var_list_lock))
	{
		free(var_list);
		var_list_c = 0;
		return -1;
	}
	
	return 0;
}


/******************************************************************************/
/** Quit using this library. */
void var_quit(void)
{
	struct var_item *v, *v2;
	int i;

	/* Return, if lib not initialized yet. */
	if (!var_list) return;
	
	lock_write(&var_list_lock);

	for (i = 0; i < var_list_c; i++)
	{
		for (v = var_list[i].first; v; )
		{
			v2 = v;
			v = (struct var_item *)v->next;
			_v_free(v2);
			free(v2);
		}
	}
	
	if (var_list) free(var_list);
	var_list = NULL;
	var_list_c = 0;
	
	lock_destroy(&var_list_lock);
}


/******************************************************************************/
/** Dump debug info about contents of lists and their variables. */
void var_dump(void)
{
	struct var_item *v;
	int i, j;
	char *type, content[MAX_STRING];

	/* Return, if lib not initialized yet. */
	if (!var_list)
	{
		printf("library is empty\n");
		return;
	}
	
	for (i = 0; i < var_list_c; i++)
	{
		printf("** %d. printing items in list (name \'%s\', item count %d)\n", (int)i, var_list[i].name, (int)var_list[i].count);
		for (j = 0, v = var_list[i].first; v; v = (struct var_item *)v->next, j++)
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
			}
			
			memset(content, 0, sizeof(content));
			strncpy(content, v->data, 20);
			if (strlen(v->data) > strlen(content)) strcat(content, "...");
			printf("    %d. name \'%s\', type \'%s\', size %d, content \'%s\'\n", (int)j, v->key, type, (int)v->size, content);
		}
	}
}


/******************************************************************************/
/**
 * Allocate new item in variables.
 * @param name Optional name for new variable list. Can be NULL.
 * @return Index of new list or -1 on errors.
 */
var_list_t varl_new(char *name)
{
	int size, err;
	var_list_t list = -1;
	
	/* Return error, if lib not initialized yet. */
	if (!var_list) return -1;

	/* check for existing list with this name */
	list = varl_find(name);
	if (list > -1) return list;
	
	lock_write(&var_list_lock);
	
	/* Re-alloc space for new item. */
	size = VAR_LIST_SIZE * (var_list_c + 1);
	var_list = (struct var_list *)realloc(var_list, size);
	if (!var_list)
	{
		err = -1;
		goto out_err;
	}
	
	/* Setup new item. */
	memset(&var_list[var_list_c], 0, VAR_LIST_SIZE);
	if (name) STRCPY(var_list[var_list_c].name, name);
	var_list_c++;
	
	list = (var_list_c - 1);

out_err:
	lock_unlock(&var_list_lock);
	return list;
}


/******************************************************************************/
/**
 * Find variable list with its name. This function can be used to find only
 * named lists. If named list is not found, default list 0 is returned.
 * Default list has no name, so either caller can use returned group number
 * without caring whether right list is found or detect unfound list from
 * return value 0.
 *
 * @param name Name of list given when calling varl_new().
 * @return Index of item found or -1 if not found.
 */
var_list_t varl_find(char *name)
{
	var_list_t i;
	
	/* Return error, if lib not initialized yet. */
	if (!var_list) return -1;
	/* Return first list, if no name is given. */
	if (!name) return -1;
	if (strlen(name) < 1) return 0;
	
	lock_read(&var_list_lock);
	
	/* Find list. */
	for (i = 0; i < var_list_c; i++)
	{
		if (strcmp(var_list[i].name, name) == 0) goto out_err;
	}
	i = -1;

out_err:
	lock_unlock(&var_list_lock);
	return i;
}


/******************************************************************************/
int varl_rename(var_list_t list, char *name)
{
	if (!var_list) return -1;
	if (list >= var_list_c || list < 0) return -1;
	lock_write(&var_list_lock);
	STRCPY(var_list[list].name, name);
	lock_unlock(&var_list_lock);
	return 0;
}


/******************************************************************************/
/**
 * Set variable as ascii string.
 * This function searches, if item within this name is already found,
 * and replaces it, or creates new one if old not found.
 * List ID should be some of following:
 * 0 which is default list, list ID returned by varl_new() (>0),
 * or -1 which means all lists. If -1 is defined, then all lists are searched
 * for occurenses of name and data for all found items is set to given.
 * In this case (-1), no new items are created in lists.
 *
 * @param list ID of list to be used returned by varl_new().
 *             0 can be used as default list, and it is always 
 *             available after var_init() has been called.
 *             See more from description.
 * @param name Name of item to be set.
 * @param string printf() style formatted string and arguments following.
 * @return Returns 0 on success, -1 on errors.
 */
int varl_set_str(var_list_t list, char *name, char *string, ...)
{
	int size, create, err;
	struct var_item *v;
	va_list args;
	char *newstr = NULL;

	/* Format given ascii data. */
	va_start(args, string);
	size = vasprintf(&newstr, string, args);
	va_end(args);
	if (size < 0) return -1;
	size++; /* Include terminating null char in size. */
	
	/* Go trough requested lists. */
	err = _v_list_set(list, name, newstr, size, VAR_TYPE_STR);

	free(newstr);

	return err;
}


/******************************************************************************/
/**
 * As varl_set_str, but set variable as number.
 *
 * @param list ID of list to be used.
 * @param name Name of item to be set.
 * @param num Number to be set.
 * @return Returns 0 on success, -1 on errors.
 */
int varl_set_num(var_list_t list, char *name, double num)
{
	int size, err;
	char *str;

	/* Format given number as ascii data. */
	size = asprintf(&str, "%lf", num);
	if (size < 0) return -1;
	size++; /* Include terminating null char in size. */
	
	/* Go trough requested lists. */
	err = _v_list_set(list, name, str, size, VAR_TYPE_NUM);

	free(str);

	return err;
}


/******************************************************************************/
/**
 * As varl_set_num, but set variable as integer number, not double.
 *
 * @param list ID of list to be used.
 * @param name Name of item to be set.
 * @param num Number to be set.
 * @return Returns 0 on success, -1 on errors.
 */
int varl_set_int(var_list_t list, char *name, int num)
{
	return varl_set_num(list, name, (double)num);
}


/******************************************************************************/
int varl_set_bin(var_list_t list, const char *name, void *data, size_t size)
{
	int err;
	err = _v_list_set(list, name, data, size, VAR_TYPE_BIN);
	return err;
}


/******************************************************************************/
void varl_rm(var_list_t list, const char *name)
{
	struct var_item *v;
	
	lock_write(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		_v_free(v);
	}
	lock_unlock(&var_list_lock);
}


/******************************************************************************/
/**
 * Get data as ascii string (if possible). List ID can be -1, in which case
 * all list are searched and first occurrense of name item is returned.
 *
 * @param list ID of list to be used returned by varl_new().
 * @param name Name of data string to get.
 * @return Pointer to ascii string, or pointer to "" (empty string), if
 *         no such item found or if item cannot be converted as string.
 */
const char *varl_get_str(var_list_t list, char *name)
{
	int i, n;
	struct var_item *v;
	char *p = var_empty_string;
	
	/* Return, if lib not initialized yet. */
	if (!var_list) return var_empty_string;
	/* Return, if name is invalid. */
	if (!name) return var_empty_string;

	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		if (v->type == VAR_TYPE_STR)
		{
			p = v->data;
		}
		else if (v->type == VAR_TYPE_NUM) p = v->data;
		else p = var_empty_string;
	}
	
	/* expand variables */
	while (vopt[VAR_OPT_EXPAND])
	{
		char *e, *ep, *epsave, *format, **arglist;
		struct var_item *v;
		int c;
		
		epsave = ep = strdup(p);
		if (!ep) break;
		format = malloc(strlen(p));
		if (!format)
		{
			free(ep);
			break;
		}
		*format = '\0';
		
		/* find expandable variables */
		arglist = NULL;
		c = 0;
		while ((e = strpbrk(ep, VAR_CHARS)))
		{
			v = _v_find_best(list, e + 1);
			if (v)
			{
				*e = '\0';
				strcat(format, ep);
				strcat(format, "%s");
				printf("1: %s, %d, %s\n", v->key, atoi(v->data), format);
				c++;
				arglist = realloc(arglist, c * sizeof(*arglist));
				if (!arglist) break;
				arglist[c - 1] = v->data;
				ep = e + strlen(v->key) + 1;
				printf("%s, %s\n", ep, v->data);
			}
			else
			{
				ep = e + 1;
			}
		}
		strcat(format, ep);
		
		/* expand */
		if (arglist)
		{
			asprintf(&p, format, *arglist);
			printf("%s, %s, %s\n", p, format, arglist[0]);
			free(arglist);
		}
		
		free(format);
		free(epsave);
		break;
	}

	lock_unlock(&var_list_lock);
	
	return p;
}


/******************************************************************************/
/**
 * Get data as double, if invalid zero is returned. List ID can be -1, in
 * which case all list are searched and first occurrense of name item is
 * returned.
 *
 * @param list ID of list to be used returned by varl_new().
 * @param name Name of data string to get.
 * @return Value of string as double, or zero on errors.
 */
double varl_get_num(var_list_t list, char *name)
{
	const char *str = varl_get_str(list, name);
	return atof(str);
}


/******************************************************************************/
/**
 * Get data as integer, if invalid zero is returned. List ID can be -1, in
 * which case all list are searched and first occurrense of name item is
 * returned.
 *
 * @param list ID of list to be used returned by varl_new().
 * @param name Name of data string to get.
 * @return Value of string as integer, or zero on errors.
 */
int varl_get_int(var_list_t list, char *name)
{
	const char *str = varl_get_str(list, name);
	return atoi(str);
}


/******************************************************************************/
/**
 * Compare variable value to given.
 *
 * @param list List ID which to used in search.
 * @param name Name of data string to get.
 * @param value Value to be compared against.
 * @return 1 if value is same, zero if it's not.
 */
int varl_is_str(var_list_t list, char *name, char *value)
{
	struct var_item *v;
	int err = 0;

	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
		case VAR_TYPE_NUM:
			if (strcmp(v->data, value) == 0) err = 1;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Compare variable value to given integer value.
 *
 * @param list List ID which to used in search.
 * @param name Name of data string to get.
 * @param value Value to be compared against.
 * @return 1 if value is same, zero if it's not.
 */
int varl_is_num(var_list_t list, char *name, double value)
{
	struct var_item *v;
	int err = 0;
	
	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
		case VAR_TYPE_NUM:
			if (atof(v->data) == value) err = 1;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Compare variable value to given integer value.
 *
 * @param list List ID which to used in search.
 * @param name Name of data string to get.
 * @param value Value to be compared against.
 * @return 1 if value is same, zero if it's not.
 */
int varl_is_int(var_list_t list, char *name, int value)
{
	struct var_item *v;
	int err = 0;
	
	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
		case VAR_TYPE_NUM:
			if (atoi(v->data) == value) err = 1;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Check whether variable has some content or is empty.
 * Variable is defined as empty also, if it is not found at all.
 * In case of different types, empty means following:
 * <br>ascii: if string length is 0, then return empty.
 * <br>numeric: returns always not empty.
 * <br>binary: if data length is less than 1.
 *
 * @param list List ID which to used in search.
 * @param name Name of data string to get.
 * @return 0 if variable is non-empty string, 1 if it is empty.
 */
int varl_is_empty(var_list_t list, char *name)
{
	struct var_item *v;
	int err = 1;

	if (list < 0 || list >= var_list_c) return 1;
	
	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
		case VAR_TYPE_NUM:
			if (strlen(v->data) > 0) err = 0;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Return 1 if the contents can be defined as "true". 0 (false) otherwise.
 * If the item is binary, false is returned always. Also if the item
 * does not exists, false is returned.
 *
 * <br>Contents can be defined as true in following cases:
 * <br>The item is numerical and has value other than zero.
 * <br>The item is string and the content is not empty string nor "0"
 *
 * @param list List ID which to used in search.
 * @param name Variable identifier key.
 * @return Length of the variables string presentation.
 */
int varl_is(var_list_t list, char *name)
{
	struct var_item *v;
	int err = 0;
	
	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
			if (strlen(v->data) < 1) err = 0;
			else if (strcmp(v->data, "0") == 0) err = 0;
			else err = 1;
			break;
		case VAR_TYPE_NUM:
			err = (atof(v->data) == 0.0f) ? 0 : 1;
			break;
		default:
			err = 0;
			break;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Set variable as true. Same as setting value to (int)1.
 *
 * @param list List ID which to used in search.
 * @param key Variable identifier key.
 */
void varl_true(var_list_t list, char *key)
{
	varl_set_num(list, key, (double)1);
}


/******************************************************************************/
/**
 * Set variable as false. Same as setting value to (int)0.
 *
 * @param list List ID which to used in search.
 * @param key Variable identifier key.
 */
void varl_false(var_list_t list, char *key)
{
	varl_set_num(list, key, (double)0);
}


/******************************************************************************/
/**
 * Return length of the variable, if it is a string. Error (-1) otherwise.
 *
 * @param list List ID which to used in search.
 * @param name Name of data string to get.
 * @return Length of the variables string presentation.
 */
int varl_strlen(var_list_t list, char *name)
{
	struct var_item *v;
	int err = 0;
	
	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (v)
	{
		switch (v->type)
		{
		case VAR_TYPE_STR:
		case VAR_TYPE_NUM:
			err = strlen(v->data);
			break;
		default:
			err = -1;
			break;
		}
	}
	lock_unlock(&var_list_lock);
	
	return err;
}


/******************************************************************************/
/**
 * Parse variable into buffer.
 *
 * @param list List ID which to search for the variable.
 * @param name Name of variable.
 * @param lists List IDs, which are used when searhing variables in string.
 * @param lc Number of items in lists.
 * @return Allocated pointer to parsed string.
 *         Must be freed after usage.
 *         Returns empty string on errors also.
 */
const char *varl_parsev(var_list_t list, char *name, int *lists, int lc)
{
	int i;
	char *result = NULL;
	struct var_item *v;
	struct var_list l;

	/* Return, if lib not initialized yet. */
	if (!var_list) return var_empty_string;
	/* Return, if name is invalid. */
	if (!name) return var_empty_string;

	lock_read(&var_list_lock);
	v = _v_find(list, name);
	if (!v) goto out_err;
	
	/* Parse variable content. */
	memset(&l, 0, sizeof(l));
	result = _v_parse(v, &l, lists, lc);

out_err:
	lock_unlock(&var_list_lock);
	if (!result) result = var_empty_string;
	return result;
}


/******************************************************************************/
/**
 * Parse variable into buffer.
 *
 * @param list List ID which to search for the variable.
 * @param name Name of variable.
 * @param argv List IDs, which are used when searhing variables in string.
               End list with -1, or if this is the only ID given,
               then all lists are used.
 * @return Return result, or 0 on errors.
 */
const char *varl_parse(var_list_t list, char *name, void *argv, ...)
{
	int lc, *lists, all, i;
	const char *result = NULL;
	
	/* Calculate lists count. */
	for (lc = 0; (long)((&argv)[lc]) != -1; lc++);

	/* If no lists, then use all. */
	if (lc < 1)
	{
		all = -1;
		lists = &all;
		lc = 1;
	}
	/* Retrieve lists, if given. */
	else 
	{
		all = 0;
		lists = (int *)malloc(sizeof(*lists) * lc);
		if (!lists) goto out_err;
		for (i = 0; i < lc; i++) lists[i] = (int)((&argv)[i]);
	}

	/* Parse variable content. */
	result = varl_parsev(list, name, lists, lc);
	
out_err:
	if (lists && !all) free(lists);
	if (!result) result = var_empty_string;
	return result;
}


/******************************************************************************/
/**
 * First parse, then calculate variable and return result.
 *
 * @param list List ID which to search for the variable.
 * @param name Name of variable.
 * @param argv List IDs, which are used when searhing variables in string.
 *             End list with -1, or if this is the only ID given,
 *             then all lists are used.
 * @return Return result, or 0 on errors.
 */
double varl_calc(var_list_t list, char *name, void *argv, ...)
{
	int lc, *lists, all, i;
	const char *res = NULL;
	double result = 0.0F;
	
	/* Calculate lists count. */
	for (lc = 0; (long)((&argv)[lc]) != -1; lc++);

	/* If no lists, then use all. */
	if (lc < 1)
	{
		all = -1;
		lists = &all;
		lc = 1;
	}
	/* Retrieve lists, if given. */
	else 
	{
		all = 0;
		lists = (int *)malloc(sizeof(int) * lc);
		if (!lists) goto out_err;
		for (i = 0; i < lc; i++) lists[i] = (int)((&argv)[i]);
	}

	/* Parse variable content. */
	res = varl_parsev(list, name, lists, lc);

	/* Calculate it's value. */
	result = strmath_calc(res, NULL);

out_err:
	if (res) free(res);
	if (lists && !all) free(lists);
	return result;
}


/******************************************************************************/
/**
 * Free given data.
 *
 * @param ptr Pointer to data to be freed.
 */
void varl_free(void *ptr)
{
	/* If NULL pointer somewhy given. */
	if (!ptr) return;
	/* If pointer is internal empty string pointer. */
	if (ptr == var_empty_string) return;
	free(ptr);
}


/******************************************************************************/
int varl_file(const char *file, void *lists)
{
	return varl_file_prefunc(file, lists, NULL);
}


/******************************************************************************/
int varl_file_prefunc(const char *file, void *lists,
	int (*prefunc)(var_list_t, const char *, const char *))
{
	FILE *f;
	char *line = NULL, *cur, *name, *value, *middle, *varval;
	ssize_t n = 0;
	int err = 0;
	var_list_t list = 0;

	/* Open file. */
	if (!file) return -1;
	f = fopen(file, "r");
	if (!f) return -1;

	for (line = NULL, n = 0, list = 0; ; )
	{
		/* Get new line from stream. */
		err = getline(&line, &n, f);
		if (err < 1) break;
		
		/* Trim line. */
		cur = _v_trim(line);
		if (strlen(cur) < 1) continue;
		
		/* Dont parse commented lines. */
		if (*cur == '#') continue;
		
		/* Parse new list. */
		if (*cur == '[')
		{
			err = sscanf(cur, "[%a[^]\n]", &name);
			if (err == 1)
			{
				//printf("%d, %s\n", err, name);
				list = varl_new(name);
				free(name);
			}
			continue;
		}
		
		/* Parse variable. */
		name = NULL; middle = NULL; value = NULL;
		err = sscanf(cur, "%a[^ =\t\n]%a[ =\t]%a[^\n]", &name, &middle, &value);
		if (err == 3 && strchr(middle, (int)'='))
		{
			err = (int)value[0];
			if (err == '\"' || err == '\'')
			{
				varval = strchr(value + 1, err);
				if (varval)
				{
					varval[0] = '\0';
					varval = value + 1;
				}
				else varval = value;
			}
			else varval = value;

			int done = 0;
			if (prefunc)
			{
				done = prefunc(list, name, varval);
			}
//			printf("%d, %s = %s\n", err, name, varval);
			if (!done)
			{
				varl_set_str(list, name, varval);
			}
		}
		if (name) free(name);
		if (middle) free(middle);
		if (value) free(value);
	}

	fclose(f);
	if (line) free(line);
	return 0;
}


/******************************************************************************/
int varl_count(var_list_t index)
{
	if (index >= var_list_c || index < 0) return 0;
	return var_list[index].count;
}


/******************************************************************************/
void varl_reset(var_list_t list)
{
	if (list >= var_list_c || list < 0) return;
	lock_write(&var_list_lock);
	struct var_list *l = &var_list[list];
	l->current = l->first;
	lock_unlock(&var_list_lock);
}


/******************************************************************************/
char *varl_each(var_list_t list, int *type, void **value, size_t *size)
{
	char *key = NULL;

	lock_write(&var_list_lock);
	struct var_list *l = &var_list[list];
	if (!l->current)
	{
		l->current = l->first;
	}
	else
	{
		key = strdup(l->current->key);
		if (type) *type = l->current->type;
		if (value)
		{
			*value = malloc(l->current->size);
			memcpy(*value, l->current->data, l->current->size);
		}
		if (size) *size = l->current>size;
		l->current = l->current->next;
	}

	lock_unlock(&var_list_lock);
	return key;
}


/******************************************************************************/
char **varl_get_keys(var_list_t index)
{
	char **result = NULL;
	struct var_list *list = NULL;
	struct var_item *v;
	int i;
	
	if (index >= var_list_c) return NULL;
	lock_read(&var_list_lock);
	list = &var_list[index];
	result = (char **)malloc(sizeof(char *) * list->count);
	for (i = 0, v = list->first; v; v = (struct var_item *)v->next, i++)
	{
		result[i] = v->key;
	}
	lock_unlock(&var_list_lock);
	return result;
}


/******************************************************************************/
char **varl_get_all(var_list_t index)
{
	char **result = NULL;
	struct var_list *list = NULL;
	struct var_item *v;
	int i;

	if (index >= var_list_c) return NULL;
	lock_read(&var_list_lock);
	list = &var_list[index];
	result = (char **)malloc(sizeof(char *) * list->count * 2);
	for (i = 0, v = list->first; v; v = (struct var_item *)v->next, i += 2)
	{
		result[i] = v->key;
		result[i + 1] = v->data;
	}
	lock_unlock(&var_list_lock);
	return result;
}

