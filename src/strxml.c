/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ddebug/synchro.h>
#include "strxml.h"


#ifdef _DEBUG
#define PRINTF printf
#else
#define PRINTF
#endif


/******************************************************************************/
struct var_xml_path
{
    char *name;
    struct var_xml_path *prev, *next;
};

struct var_xml_node
{
    char *name;
    char *content;
    int depth;
    char *msg;
    int type;

    struct var_xml_node *a;
    int ac;
    struct var_xml_node **c;
    int cc;
};

struct var_xml_tree
{
    char *version;
    char *encoding;

    struct var_xml_node *root;
};


/******************************************************************************/
/* Xml array container. */
static struct _list *xml_list = NULL;
/* Xml array size. */
static int xml_list_c = 0;
/* Internal lock for xml. */
static lock_t xml_list_lock;
/* Constant empty variable string for internal use. */
static char *xml_empty_string = "";


/******************************************************************************/
/* FUNCTIONS */
char *_v_trim(char *str);
char *_v_trim_all(char *str);


/******************************************************************************/
/** Internal help routine: Add message to node's messages. */
void _xml_add_msg(struct var_xml_node *root, const char *str, ...)
{
    char *msg = NULL, *add = NULL;
    va_list args;

    va_start(args, str);
    vasprintf(&add, str, args);
    va_end(args);

//     PRINTF("%s\n", add);

    if (root->msg)
    {
        asprintf(&msg, "%s\n%s", root->msg, add);
        free(root->msg);
    }
    else asprintf(&msg, "%s", add);

    root->msg = msg;
    if (add) free(add);
}


/******************************************************************************/
/**
 *
 */
struct var_xml_node *_xml_child_find(struct var_xml_node *root, const char *child)
{
    int i;
    struct var_xml_node *node = NULL;

    if (strcmp(root->name, child) == 0) return root;
    for (i = 0; i < root->cc; i++)
    {
        node = _xml_child_find(root->c[i], child);
        if (node) break;
    }

    return node;
}


/******************************************************************************/
/**
 *
 */
struct var_xml_node *_xml_attr_find(struct var_xml_node *root, const char *attr)
{
    int i;

    for (i = 0; i < root->ac; i++)
    {
        if (strcmp(root->a[i].name, attr) == 0) return &root->a[i];
    }

    return NULL;
}


/******************************************************************************/
/**
 * Add new attribute to node.
 */
int _xml_attr_add(struct var_xml_node *node, const char *name, const char *content)
{
    struct var_xml_node *a;

    /* Allocate memory for new attribute. */
    node->a = realloc(node->a, (node->ac + 1) * sizeof(*node->a));
    if (!node->a) return -1;
    a = &node->a[node->ac];
    node->ac++;

    /* Setup new attribute. */
    memset(a, 0, sizeof(*a));
    a->name = strdup(name);
    a->content = strdup(content);
}


/******************************************************************************/
/**
 *
 */
int _xml_attr_parse(const char *attr, struct var_xml_node **a, int *ac)
{
    int err = 0;
    char *name = NULL, *value = NULL, *rest = NULL;
    struct var_xml_node *aa;

    err = sscanf(attr, "%a[^=]=\"%a[^\"]\"%*[ ]%a[^\n]", &name, &value, &rest);
//    PRINTF("scanned attr with name \"%s\" and value \"%s\" (end \"%s\")\n", name, value, rest);
    if (err < 2) return 0;
//    value = _v_trim(value);

    /* Allocate memory for new attribute. */
    (*a) = realloc(*a, (*ac + 1) * sizeof(**a));
    if (!(*a)) return -1;
    aa = &((*a)[*ac]);
    (*ac)++;

    /* Setup new attribute. */
    memset(aa, 0, sizeof(*aa));
    aa->name = name;
    aa->content = value;

    if (!rest) return 0;
    _xml_attr_parse(rest, a, ac);
    free(rest);
    return 0;
}


/******************************************************************************/
/**
 *
 */
struct var_xml_node *_xml_node_add(struct var_xml_node *root, int depth, const char *name)
{
    struct var_xml_node *node = NULL;

    /* Allocate new node. */
    node = malloc(sizeof(*node));
    if (!node) return NULL;

    /* Setup new node. */
    memset(node, 0, sizeof(*node));
    node->depth = depth;
    node->content = xml_empty_string;
    node->name = strdup(name); /* Not checking errors */

    /* Add node to its root, if such. */
    if (root)
    {
        root->c = realloc(root->c, (root->cc + 1) * sizeof(*root->c));
        if (!root->c) return NULL;
        root->c[root->cc] = node;
        root->cc++;
    }

    return node;
}


/******************************************************************************/
/**
 *
 * @return -1 on errors, 0 on success.
 */
int _xml_root(const char *line, struct var_xml_node **root, int depth)
{
    int err = 0;
    char *attr = NULL;
    struct var_xml_node *a = NULL;
    int ac = 0, i;

    /* Check for invalid root. */
    if (*root || depth != 0) return -1;

    err = sscanf(line, "<?xml %a[^?]?>", &attr);
    if (err != 1) return 0;

//    PRINTF("parsed root node\n");
    err = _xml_attr_parse(attr, &a, &ac);

//    PRINTF(" found %d attributes:\n", ac);
//    for (i = 0; i < ac; i++)
    {
//        PRINTF("  %d. name \"%s\", value \"%s\"\n", i, a[i].name, a[i].content);
    }

    *root = _xml_node_add(NULL, depth, "xml");
    if (*root)
    {
        (*root)->a = a;
        (*root)->ac = ac;
        (*root)->content = xml_empty_string;
    }

    if (attr) free(attr);
    return 0;
}


/******************************************************************************/
/**
 *
 * @return NULL if node was closed (<.../>) or on errors,
 *         non NULL if content exists (<...>).
 */
struct var_xml_node *_xml_node(const char *line, struct var_xml_node *root, int depth)
{
    int err = 0;
    char *name = NULL, *attr = NULL, rest[4], *p, *line_copy = NULL;
    struct var_xml_node *a = NULL, *node = NULL;
    int ac = 0, i;

    /* Check for invalid root. */
    if (!root || depth < 1) return NULL;

    /* copy given line */
    line_copy = strdup(line);
    if (!line_copy) return NULL;

    /* Find the start of the nodes name */
    p = _v_trim(line_copy + 1);
//     PRINTF(":%s\n", p);

    err = sscanf(p, "%a[^ \r\n\t<>]%*[ \r\n\t]%a[^\n]", &name, &attr);
    if (err < 1)
    {
        if (line_copy) free(line_copy);
        return NULL;
    }

    strcpy(rest, ">");
    if (attr == NULL) {
        /* ... */
    } else if (attr[strlen(attr) - 2] == '/') {
        attr[strlen(attr) - 2] = '\0';
        strcpy(rest, "/>");
    } else if (attr[strlen(attr) - 1] == '>') {
        attr[strlen(attr) - 1] = '\0';
    } else {
        return NULL;
    }
    printf("'%s' '%s' '%s' '%s'\n", p, name, attr, rest);

//     PRINTF("parsed node with name \"%s\", attr \"%s\", (end \"%s\"), parent \"%s\", depth %d\n", name, attr, rest, root->name, depth);
    if (attr) err = _xml_attr_parse(attr, &a, &ac);

//     PRINTF(" found %d attributes:\n", ac);
    for (i = 0; i < ac; i++)
    {
//         PRINTF("  %d. name \"%s\", value \"%s\"\n", i, a[i].name, a[i].content);
    }

    node = _xml_node_add(root, depth, name);
    if (node)
    {
        node->a = a;
        node->ac = ac;
    }

    if (!node || !rest);
    else if (rest[0] == '/') node = NULL;

    if (name) free(name);
    if (attr) free(attr);
    if (line_copy) free(line_copy);
    return node;
}


/******************************************************************************/
/**
 * Check end of node and print warning, if it does not match.
 *
 * @return 0 on success, -1 on errors.
 */
int _xml_node_end(DIO *f, const char *line, struct var_xml_node *root)
{
    int err = 0;
    int i;
    char *name = NULL, *warning = NULL;

    sscanf(line, "</%a[^<>\n]", &name);
    if (strcmp(name, root->name) == 0) goto out_err;

    _xml_add_msg(root, "warning: node's <%s> end </%s> does not match or invalid termination of node %s", root->name, name, root->name);

    for (i = strlen(line) - 1; i >= 0; i--) {
        dio_ungetc((int)line[i], f);
    }

out_err:
    if (name) free(name);
    return err;
}


/******************************************************************************/
/**
 *
 */
int _xml_parse_file(DIO *f, struct var_xml_node **prev, int depth, int state)
{
    char *line = NULL, *cur;
    int err = 0, n = 0, delim, ch;
    struct var_xml_node *node;

    /* Get new line from stream. */
    delim = (int)'>';
    if (state == 0 || state == -1);
    else if (state == 1)
    {
        for (ch = dio_getc(f); isspace(ch); ch = dio_getc(f));
        dio_ungetc(ch, f);
        if (ch == '<') delim = (int)'>';
        else delim = (int)'<';
    }
    err = (int)dio_getdelim(&line, &n, delim, f);
    if (err < 1)
    {
        if (depth > 0)
        {
            _xml_add_msg(*prev, "warning: EOF too early, parsing node <%s>!", (*prev)->name);
        }
        if (line) free(line);
        return 1;
    }
    if (delim == (int)'<' && line[strlen(line) - 1] == '<')
    {
        dio_ungetc('<', f);
        line[strlen(line) - 1] = '\0';
    }

    /* trim line */
    cur = _v_trim_all(line);
//    if (strlen(cur) < 1) return _xml_parse_file(f, prev, depth, 0);

    /* check if this is start of something */
    if (cur[0] == '<')
    {
        /* root node? */
        if (cur[1] == '?') {
            err = _xml_root(cur, prev, depth);
            if (line) free(line);
            if (err) return 0;
            return _xml_parse_file(f, prev, depth + 1, 0);
        }
        /* comment? */
        if (cur[1] == '!') {
            if (cur[2] == '-' && cur[3] == '-') {
                delim = (int)'>';
                while (1) {
                    err = (int)dio_getdelim(&line, &n, delim, f);
                    if (err < 1) {
                        _xml_add_msg(*prev, "warning: EOF too early, document block not closed!");
                    }
                    if (strlen(line) >= 3) {
                        size_t z = strlen(line) - 3;
                        if (line[z] == '-' && line[z + 1] == '-' && line[z + 2] == '>') {
                            break;
                        }
                    }
                }
                return _xml_parse_file(f, prev, depth, state);
            } else {
                return _xml_parse_file(f, prev, depth, state);
            }
        }
        /* end of node? */
        if (cur[1] == '/') {
            _xml_node_end(f, cur, *prev);
            if (line) free(line);
            return 1;
        }
        /* normal node then. */
        else
        {
            if (state == -1)
            {
                err = _xml_root("<?xml version=\"1.0\"?>", prev, 0);
                if (err) return 0;
                _xml_add_msg(*prev, "warning: <?xml ...> start node missing! Added it automatically.");
                depth++;
            }
            node = _xml_node(cur, *prev, depth);
            if (node) while (!_xml_parse_file(f, &node, depth + 1, 1));
        }

        goto out_err;
    }

//    PRINTF("### line (%d): %s\n", depth, cur);

    (*prev)->content = strdup(cur);

out_err:
    if (line) free(line);
    if (err) return 0;
    return _xml_parse_file(f, prev, depth, 0);
}


/******************************************************************************/
var_xml_t var_xml_load(const char *file)
{
    DIO *f;
    struct var_xml_node *root = NULL;

    if (!file) return NULL;
    f = dio_fopen(file, "r");
    if (!f) return NULL;

    _xml_parse_file(f, &root, 0, -1);

    dio_close(f);
    return root;
}


/******************************************************************************/
var_xml_t var_xml_parse(const char *data, size_t len)
{
    DIO *f;
    struct var_xml_node *root = NULL;

    f = dio_mopen(data, len, 1);
    if (!f) return NULL;

    _xml_parse_file(f, &root, 0, -1);

    dio_close(f);
    return root;
}


/******************************************************************************/
void _var_xml_dump(FILE *f, struct var_xml_node *node)
{
//    struct var_xml_node *node;
    int i, depth;
    char pad[100];

    depth = node->depth;
    memset(pad, 0, sizeof(pad));
    for (i = 0; i < depth; i++) pad[i] = ' ';

    fprintf(f, "%s[%s:", pad, node->name);
    for (i = 0; i < node->ac; i++)
    {
        fprintf(f, " %s=\"%s\"", node->a[i].name, node->a[i].content);
        if ((i + 1) < node->ac) fprintf(f, ",");
    }
    fprintf(f, "]");
    fprintf(f, " \"%s\"\n", node->content);
    if (node->msg) fprintf(f, "%s\n", node->msg);
    for (i = 0; i < node->cc; i++)
    {
        _var_xml_dump(f, node->c[i]);
    }
}


/******************************************************************************/
void var_xml_dump(FILE *f, var_xml_t xml)
{
    _var_xml_dump(f, xml);
}


/******************************************************************************/
void var_xml_free(var_xml_t node)
{
    int i, depth;

    for (i = 0; i < node->ac; i++)
    {
        free(node->a[i].name);
        free(node->a[i].content);
    }
    for (i = 0; i < node->cc; i++)
    {
        var_xml_free(node->c[i]);
    }

    if (node->a) free(node->a);
    if (node->c) free(node->c);
    if (node->content && node->content != xml_empty_string) free(node->content);
    if (node->name) free(node->name);
    if (node->msg) free(node->msg);
    if (node) free(node);
}


/******************************************************************************/
/**
 * Find first node with given name from XML list.
 *
 * @param root Root node.
 * @param child Name of child node.
 * @return Child node, or NULL if not found.
 */
var_xml_t var_xml_node_find(var_xml_t root, const char *child)
{
    var_xml_t node;

    node = _xml_child_find(root, child);

    return node;
}


/******************************************************************************/
/**
 * Find first attribute with given name from XML node.
 *
 * @param node Node.
 * @param attr Name of attribute.
 * @return Attribute, or NULL if not found.
 */
var_xml_t var_xml_attr_find(var_xml_t node, const char *attr)
{
    var_xml_t a;

    a = _xml_attr_find(node, attr);

    return a;
}


/******************************************************************************/
/**
 * Travel trough child nodes of a root node.
 *
 * @param root Root node.
 * @param callback Callback function which is called for every node travelled.
 *                 This function must return 0 to continue travelling.
 * @param name Child must match this name. Set to NULL if not used.
 * @param recursively 1 if travel recursively trough child nodes childs also.
 * @param data User data.
 * @return Number of nodes gone trough. If travelling was break by callback
 *         returning non-zero, negative of number of travelled nodes is returned.
 */
int var_xml_travel(var_xml_t root,
                   int (*f)(var_xml_t, var_xml_t, int, void *),
                   const char *name,
                   int recursively, void *data)
{
    int err = 0, i, n = 0;

    for (i = 0; i < root->cc; i++)
    {
        /* Check if name matches (in case it has been given). */
        err = 0;
        if (name) if (strcmp(root->c[i]->name, name) != 0) err = 1;
        /* If all ok, call callback. */
        if (!err) err = f(root, root->c[i], root->depth, data);
        n++;
        if (err)
        {
            n *= -1;
            goto out_err;
        }

        /* Travel child nodes, if recursion wanted. */
        if (recursively)
        {
            err = var_xml_travel(root->c[i], f, name, recursively, data);
            if (err < 0)
            {
                n *= -1;
                n += err;
                goto out_err;
            }
            else n += err;
        }
    }

out_err:
    return n;
}


/******************************************************************************/
/** Return name of XML node. */
char *var_xml_name(var_xml_t node) { return node->name; }


/******************************************************************************/
/** Return content of XML node. */
char *var_xml_str(var_xml_t node) { return node->content; }


/******************************************************************************/
/** Return depth of XML node in XML tree. */
int var_xml_depth(var_xml_t node) { return node->depth; }


/******************************************************************************/
/** Return attribute count of XML node. */
int var_xml_ac(var_xml_t node) { return node->ac; }


/******************************************************************************/
/** Return child node count of XML node. */
int var_xml_cc(var_xml_t node) { return node->cc; }


/******************************************************************************/
/** Return n:th attribute of XML node, or NULL on errors (invalid id..). */
var_xml_t var_xml_a(var_xml_t node, int id)
{
    if (id < 0 || id >= node->ac) return NULL;
    return (&node->a[id]);
}


/******************************************************************************/
/** Return n:th child node of XML node, or NULL on errors (invalid id..). */
var_xml_t var_xml_c(var_xml_t node, int id)
{
    if (id < 0 || id >= node->cc) return NULL;
    return node->c[id];
}


/******************************************************************************/
/** Return attribute value as string. */
char *var_xml_attr_str(var_xml_t node, const char *attr)
{
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a) return a->content;

    return xml_empty_string;
}


/******************************************************************************/
/** Return attribute value as double. */
double var_xml_attr_num(var_xml_t node, const char *attr)
{
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a) return atof(a->content);

    return 0.0f;
}


/******************************************************************************/
/** Return attribute value as integer. */
int var_xml_attr_int(var_xml_t node, const char *attr)
{
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a) return atoi(a->content);

    return 0;
}


/******************************************************************************/
/** Return child value as string. */
char *var_xml_child_str(var_xml_t node, const char *child)
{
    var_xml_t c;

    c = _xml_child_find(node, child);
    if (c) return c->content;

    return xml_empty_string;
}


/******************************************************************************/
/** Return child value as double. */
double var_xml_child_num(var_xml_t node, const char *child)
{
    var_xml_t c;

    c = _xml_child_find(node, child);
    if (c) return atof(c->content);

    return 0.0f;
}


/******************************************************************************/
/** Return child value as integer. */
int var_xml_child_int(var_xml_t node, const char *child)
{
    var_xml_t c;

    c = _xml_child_find(node, child);
    if (c) return atoi(c->content);

    return 0;
}


/******************************************************************************/
int var_xml_child_set_str(var_xml_t node, const char *child, const char *content)
{
    var_xml_t c;

    c = _xml_child_find(node, child);
    if (!c)
    {
        c = _xml_node_add(node, node->depth + 1, child);
    }

    if (c)
    {
        if (c->content && c->content != xml_empty_string)
        {
            free(c->content);
        }
        c->content = strdup(content);

    }

    return 0;
}

/******************************************************************************/
int var_xml_child_set_num(var_xml_t node, const char *child, double value)
{
    int err = 0;
    char *content = NULL;

    err = asprintf(&content, "%lf", value);
    if (err == -1) return -1;
    err = var_xml_child_set_str(node, child, content);
    free(content);

    return err;
}


/******************************************************************************/
int var_xml_child_set_int(var_xml_t node, const char *child, int value)
{
    int err = 0;
    char *content = NULL;

    err = asprintf(&content, "%d", value);
    if (err == -1) return -1;
    err = var_xml_child_set_str(node, child, content);
    free(content);

    return err;
}


/******************************************************************************/
/** Set attribute value string from string. */
int var_xml_attr_set_str(var_xml_t node, const char *attr, const char *content)
{
    int err = 0;
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a)
    {
        free(a->content);
        a->content = strdup(content);
    }
    else _xml_attr_add(node, attr, content);

    return err;
}


/******************************************************************************/
/** Set attribute value string from double. */
int var_xml_attr_set_num(var_xml_t node, const char *attr, double value)
{
    int err = 0;
    char *content = NULL;

    err = asprintf(&content, "%lf", value);
    if (err == -1) return -1;
    err = var_xml_attr_set_str(node, attr, content);
    free(content);

    return err;
}


/******************************************************************************/
/** Set attribute value string from integer. */
int var_xml_attr_set_int(var_xml_t node, const char *attr, int value)
{
    int err = 0;
    char *content = NULL;

    err = asprintf(&content, "%d", value);
    if (err == -1) return -1;
    err = var_xml_attr_set_str(node, attr, content);
    free(content);

    return err;
}


/******************************************************************************/
/** Find node within parent with one attribute content mathing to given. */
var_xml_t var_xml_node_find1(var_xml_t parent, const char *child,
                             const char *attr, const char *value)
{
    int i;
    var_xml_t node, a;

    for (i = 0; i < parent->cc; i++)
    {
        node = parent->c[i];
        if (strcmp(node->name, child) != 0) continue;
        a = _xml_attr_find(node, attr);
        if (!a) continue;
        if (strcmp(a->content, value) != 0) continue;
        return node;
    }

    return NULL;
}


/******************************************************************************/
/**
 * Compare whether node's attribute has given value or not.
 *
 * @param node XML node.
 * @param attr Attribute name.
 * @param value Value that attribute value should match.
 * @return 1 if attribute value and given value matches, 0 if not.
 */
int var_xml_attr_is_str(var_xml_t node, const char *attr, const char *value)
{
    int err = 0;
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a)
    {
        if (strcmp(a->content, value) == 0) return 1;
    }

    return 0;
}


/******************************************************************************/
/**
 * Compare whether node's attribute has given value or not.
 *
 * @param node XML node.
 * @param attr Attribute name.
 * @param value Value that attribute value should match.
 * @return 1 if attribute value and given value matches, 0 if not.
 */
int var_xml_attr_is_int(var_xml_t node, const char *attr, int value)
{
    int err = 0;
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a)
    {
        if (atoi(a->content) == value) return 1;
    }

    return 0;
}


/******************************************************************************/
int var_xml_child_exists(var_xml_t node, const char *child)
{
    var_xml_t c;

    c = _xml_child_find(node, child);
    if (c) return 1;
    return 0;
}


/******************************************************************************/
int var_xml_attr_exists(var_xml_t node, const char *attr)
{
    var_xml_t a;

    a = _xml_attr_find(node, attr);
    if (a) return 1;
    return 0;
}

