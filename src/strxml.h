/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRXML_H
#define _STRXML_H


/******************************************************************************/
#include <ddebug/dio.h>
#include "strvar.h"


/******************************************************************************/
struct var_xml_tree;
typedef struct var_xml_node * var_xml_t;


/******************************************************************************/
/**
 * Parse XML from file.
 *
 * @param file file which to parse
 * @return root node
 */
var_xml_t var_xml_load(const char *file);

/**
 * Parse XML from memory.
 *
 * @param data data to be parsed
 * @param len length of data
 * @return root node
 */
var_xml_t var_xml_parse(const char *data, size_t len);

/**
 * Dump contents of XML to given file (example stdio).
 */
void var_xml_dump(FILE *f, var_xml_t xml);

/**
 * Release XML.
 */
void var_xml_free(var_xml_t node);

/* Search functions. */
var_xml_t var_xml_node_find(var_xml_t, const char *);
var_xml_t var_xml_node_find1(var_xml_t, const char *, const char *, const char *);
var_xml_t var_xml_attr_find(var_xml_t, const char *);

int var_xml_ac(var_xml_t);
int var_xml_cc(var_xml_t);
var_xml_t var_xml_a(var_xml_t, int);
var_xml_t var_xml_c(var_xml_t, int);

int var_xml_travel(var_xml_t, int (*f)(var_xml_t, var_xml_t, int, void *), const char *, int, void *);

char *var_xml_parse_(var_xml_t);
double var_xml_calc(var_xml_t);
char *var_xml_attr_parse(var_xml_t, const char *);
double var_xml_attr_calc(var_xml_t, const char *);


/* Content management functions. */
char *var_xml_name(var_xml_t);
char *var_xml_str(var_xml_t);
int var_xml_depth(var_xml_t);

char *var_xml_child_str(var_xml_t, const char *);
double var_xml_child_num(var_xml_t, const char *);
int var_xml_child_int(var_xml_t, const char *);

/** Set child value as string. */
int var_xml_child_set_str(var_xml_t node, const char *child, const char *content);
/** Set child value as double. */
int var_xml_child_set_num(var_xml_t node, const char *child, double value);
/** Set child value as int. */
int var_xml_child_set_int(var_xml_t node, const char *child, int value);


/* Attribute management functions. */
char *var_xml_attr_str(var_xml_t, const char *);
double var_xml_attr_num(var_xml_t, const char *);
int var_xml_attr_int(var_xml_t, const char *);

int var_xml_attr_set_str(var_xml_t, const char *, const char *);
int var_xml_attr_set_num(var_xml_t, const char *, double);
int var_xml_attr_set_int(var_xml_t, const char *, int);

int var_xml_attr_is_str(var_xml_t, const char *, const char *);
int var_xml_attr_is_num(var_xml_t, const char *, double);
int var_xml_attr_is_int(var_xml_t, const char *, int);


/** Check if child exists. */
int var_xml_child_exists(var_xml_t node, const char *child);
/** Check if attribute exists */
int var_xml_attr_exists(var_xml_t node, const char *attr);


#endif /* _STRXML_H */
/******************************************************************************/

