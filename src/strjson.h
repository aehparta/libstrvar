/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRJSON_H
#define _STRJSON_H

#include "strxml.h"
#include <ddebug/debuglib.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>


/******************************************************************************/
#define VAR_JSON_NODE_TYPE_ROOT			0
#define VAR_JSON_NODE_TYPE_OBJECT		1
#define VAR_JSON_NODE_TYPE_ARRAY		2
#define VAR_JSON_NODE_TYPE_STRING		3
#define VAR_JSON_NODE_TYPE_INT			4
#define VAR_JSON_NODE_TYPE_BOOL			5
#define VAR_JSON_NODE_TYPE_NULL			6
#define VAR_JSON_NODE_TYPE_NUMBER		7


/******************************************************************************/
struct var_json_parse_stack
{
	int type; /* what kind of type are we parsing currently */
	struct var_json_node *node; /* current node */
	struct var_json_parse_stack *prev;

	char *key;
	char *data;
	size_t len;
	size_t size;
	int special; /* if next byte is special (usually when parsing strings after \-char) */
	wchar_t uxxxx;

	char *error;
};


struct var_json_node
{
	int type;
	wchar_t *wname;
	wchar_t *wstring;
	int value;
	double number;
	struct var_json_node **e; /* elements */
	size_t ec;
	struct var_json_node *parent;
	size_t index;

	/* for parsing json */
	size_t parse_bytes; /* holder of parsed bytes mostly for reporting errors */
	struct var_json_parse_stack *parse_stack;
};
typedef struct var_json_node var_json_t;


/******************************************************************************/
/**
 * Create json builder.
 */
int var_json_init();
var_json_t *var_json_new();
var_json_t *var_json_object(var_json_t *root, const char *key);
var_json_t *var_json_array(var_json_t *root, const char *key);
var_json_t *var_json_str(var_json_t *root, const char *key, const char *value, ...);
var_json_t *var_json_int(var_json_t *root, const char *key, int value);
var_json_t *var_json_number(var_json_t *root, const char *key, double value);
var_json_t *var_json_boolean(var_json_t *root, const char *key, int value);
var_json_t *var_json_null(var_json_t *root, const char *key);
var_json_t *var_json_find_by_key(var_json_t *root, char *key, int recursive);
#define var_json_find_by_name(root, key) var_json_find_by_key(root, key, 1)
char *var_json_to_str(var_json_t *root, size_t *len_ret);
var_json_t *var_json_copy(var_json_t *root, var_json_t *to_copy);
void var_json_free(var_json_t *root);

var_json_t *var_json_new_parser(var_json_t *root);
ssize_t var_json_parse_byte(var_json_t *root, int byte);
ssize_t var_json_parse_str(var_json_t *root, char *str);
ssize_t var_json_parse_file(var_json_t *root, char *filename);

char *var_json_get_key(var_json_t *node);
char *var_json_get_str(var_json_t *node);
int var_json_get_int(var_json_t *node);
double var_json_get_number(var_json_t *node);
int var_json_is_object(var_json_t *node);
int var_json_is_array(var_json_t *node);
int var_json_is_boolean(var_json_t *node);
int var_json_is_null(var_json_t *node);
int var_json_is_true(var_json_t *node);
int var_json_is_false(var_json_t *node);

const char *var_json_parse_error(var_json_t *root);

/******************************************************************************/
/**
 * Get root children as list. root must be either array or object.
 * @return list of nodes, must be freed using var_json_free_list().
 */
var_json_t **var_json_get_list(var_json_t *root, size_t *count);

void var_json_free_list(var_json_t **list, size_t count);

var_json_t *var_json_first_child(var_json_t *root);


#endif /* _STRJSON_H */
/******************************************************************************/

