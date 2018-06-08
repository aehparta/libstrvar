/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include <math.h>
#include "strjson.h"


/******************************************************************************/
/* parsing states: */
enum {
	JSON_PARSE_START = 0,
	JSON_PARSE_END,
	JSON_PARSE_SOMETHING,
	JSON_PARSE_OBJECT,
	JSON_PARSE_ARRAY,
	JSON_PARSE_VALUE,
	JSON_PARSE_KEY,
	JSON_PARSE_STRING,
	JSON_PARSE_NUMBER,
	JSON_PARSE_TRUE,
	JSON_PARSE_FALSE,
	JSON_PARSE_NULL,
};


/******************************************************************************/
static int json_initialized = 0;
static lock_t json_lock;


/******************************************************************************/
size_t json_ambstowcs(wchar_t **dst, const char *src)
{
	size_t len;
	mbstate_t mbs;

	memset(&mbs, 0, sizeof(mbs));
	len = mbsrtowcs(NULL, &src, 0, &mbs);
	if (len == -1) return -1;

	len++;
	*dst = (wchar_t *)malloc(len * sizeof(wchar_t));
	memset(*dst, 0, len * sizeof(wchar_t));
	memset(&mbs, 0, sizeof(mbs));
	mbsrtowcs(*dst, &src, len - 1, &mbs);

	return len;
}


/******************************************************************************/
size_t json_wcslentombslen(const wchar_t *src, int no_uxxxx)
{
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	size_t wlen = wcslen(src), i, len = 0;
	char buf[16];

	for (i = 0; i < wlen; i++) {
		if (wcschr(L"\"\\/\b\f\n\r\t", (int)src[i])) len += 2;
		else if (src[i] <= 0x007f || no_uxxxx) len += wcrtomb(buf, src[i], &mbs);
		else if (src[i] <= 0xffff) len += 6; // \uXXXX
	}

	return len;
}


/******************************************************************************/
size_t json_wcstombs(char *dst, const wchar_t *src, int no_uxxxx)
{
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	size_t wlen = wcslen(src), i, len = 0;

	for (i = 0; i < wlen; i++) {
		if (wcschr(L"\"\\/\b\f\n\r\t", (int)src[i])) {
			dst[len] = '\\';
			switch (src[i]) {
			case L'\b':
				dst[len + 1] = 'b';
				break;
			case L'\f':
				dst[len + 1] = 'f';
				break;
			case L'\n':
				dst[len + 1] = 'n';
				break;
			case L'\r':
				dst[len + 1] = 'r';
				break;
			case L'\t':
				dst[len + 1] = 't';
				break;
			default:
				dst[len + 1] = (char)src[i];
				break;
			}
			len += 2;
		} else if (src[i] <= 0x007f || no_uxxxx) len += wcrtomb(&dst[len], src[i], &mbs);
		else if (src[i] <= 0xffff) {
			sprintf(&dst[len], "\\u%04x", src[i]);
			len += 6;
		}
	}

	return len;
}


/******************************************************************************/
int var_json_init()
{
	int err = 0;
	IF_ER(json_initialized, 0);

	lock_init(&json_lock);
	json_initialized = 1;

out_err:
	return err;
}


/******************************************************************************/
var_json_t *_node_new(var_json_t *parent, int type, const char *name)
{
	if (parent) {
		switch (parent->type) {
		case VAR_JSON_NODE_TYPE_STRING:
			if (parent->wstring) {
				free(parent->wstring);
				parent->wstring = NULL;
			}
		case VAR_JSON_NODE_TYPE_INT:
		case VAR_JSON_NODE_TYPE_NUMBER:
		case VAR_JSON_NODE_TYPE_BOOL:
		case VAR_JSON_NODE_TYPE_NULL:
			parent->type = type;
			return parent;
		}
	}

	var_json_t *node = (var_json_t *)malloc(sizeof(*node));
	memset(node, 0, sizeof(*node));
	node->type = type;
	if (name != NULL) {
		json_ambstowcs(&node->wname, name);
	}
	if (parent != NULL) {
		node->parent = parent;
		node->index = parent->ec;
		parent->ec++;
		parent->e = (var_json_t **)realloc(parent->e, sizeof(var_json_t *) * parent->ec);
		parent->e[parent->ec - 1] = node;
	}
	return node;
}


/******************************************************************************/
size_t _node_to_str(var_json_t *root, char *b)
{
	int i, j;
	size_t len = 0;
	char *c = NULL, xc[256];

	if (root->wname) {
		len += json_wcslentombslen(root->wname, 0) + 2 + 3; // ""<name>" : "
		if (b) {
			(*b) = '\"';
			b++;
			b += json_wcstombs(b, root->wname, 0);
			strcpy(b, "\" : ");
			b += strlen(b);
		}
	}

	switch (root->type) {
	case VAR_JSON_NODE_TYPE_OBJECT:
	case VAR_JSON_NODE_TYPE_ARRAY:
		len += 2 + 2; // "[ " and " ]" or "{ " and " }"
		if (b) {
			if (root->type == VAR_JSON_NODE_TYPE_OBJECT) sprintf(b, "{ ");
			else sprintf(b, "[ ");
			b += 2;
		}
	case VAR_JSON_NODE_TYPE_ROOT:
		for (i = 0, j = 0; i < root->ec; i++) {
			if (!root->e[i]) continue;
			if (j > 0) {
				len += 2; // + 2 for ", "
				if (b) {
					sprintf(b, ", ");
					b += 2;
				}
			}
			size_t n = _node_to_str(root->e[i], b);
			len += n;
			if (b) b += n;
			j++;
		}
		break;
	case VAR_JSON_NODE_TYPE_STRING:
		len += json_wcslentombslen(root->wstring, 0) + 2; // 2 for quotes
		if (b) {
			(*b) = '\"';
			b++;
			b += json_wcstombs(b, root->wstring, 0);
			(*b) = '\"';
			b++;
		}
		break;
	case VAR_JSON_NODE_TYPE_INT:
		sprintf(xc, "%d", root->value);
		len += strlen(xc);
		if (b) sprintf(b, "%d", root->value);
		break;
	case VAR_JSON_NODE_TYPE_NUMBER:
		sprintf(xc, "%lf", root->number);
		len += strlen(xc);
		if (b) {
			sprintf(b, "%lf", root->number);
			char *dot = strchr(b, ',');
			if (dot) (*dot) = '.';
		}
		break;
	case VAR_JSON_NODE_TYPE_BOOL:
		len += root->value ? 4 : 5; // "true" or "false"
		if (b) sprintf(b, "%s", root->value ? "true" : "false");
		break;
	case VAR_JSON_NODE_TYPE_NULL:
		len += 4; // "null"
		if (b) sprintf(b, "null");
		break;
	}

	switch (root->type) {
	case VAR_JSON_NODE_TYPE_OBJECT:
	case VAR_JSON_NODE_TYPE_ARRAY:
		if (b) {
			if (root->type == VAR_JSON_NODE_TYPE_OBJECT) sprintf(b, " }");
			else sprintf(b, " ]");
		}
	}

	return len;
}


/******************************************************************************/
var_json_t *_node_clone(var_json_t *src, var_json_t *parent)
{
	int i, err;
	var_json_t *clone;

	if (src->type == VAR_JSON_NODE_TYPE_ROOT) {
		for (i = 0; i < src->ec; i++) {
			if (!src->e[i]) continue;
			_node_clone(src->e[i], parent);
		}
		return NULL;
	}

	clone = _node_new(parent, src->type, NULL);
	if (src->wname) clone->wname = wcsdup(src->wname);
	if (src->wstring) clone->wstring = wcsdup(src->wstring);
	clone->value = src->value;

	switch (src->type) {
	case VAR_JSON_NODE_TYPE_OBJECT:
	case VAR_JSON_NODE_TYPE_ARRAY:
		for (i = 0; i < src->ec; i++) {
			if (!src->e[i]) continue;
			_node_clone(src->e[i], clone);
		}
		break;
	}

out_err:
	return clone;
}


/******************************************************************************/
var_json_t *_node_find_by_key(var_json_t *root, wchar_t *wname, int recursive)
{
	int i;

	if (root->wname) {
		if (wcscmp(root->wname, wname) == 0) return root;
	}

	switch (root->type) {
	case VAR_JSON_NODE_TYPE_OBJECT:
	case VAR_JSON_NODE_TYPE_ARRAY:
	case VAR_JSON_NODE_TYPE_ROOT:
		for (i = 0; i < root->ec; i++) {
			if (!root->e[i]) continue;
			var_json_t *ret = NULL;
			if (recursive) ret = _node_find_by_key(root->e[i], wname, recursive);
			if (ret) return ret;
		}
		break;
	}

	return NULL;
}


/******************************************************************************/
void _node_free(var_json_t *root)
{
	int i;
	size_t len = 0;
	char *c = NULL;

	if (root->wname) free(root->wname);

	while (root->parse_stack) {
		struct var_json_parse_stack *stack = root->parse_stack;
		root->parse_stack = stack->prev;
		if (stack->key) free(stack->key);
		if (stack->data) free(stack->data);
		if (stack->error) free(stack->error);
		free(stack);
	}

	switch (root->type) {
	case VAR_JSON_NODE_TYPE_OBJECT:
	case VAR_JSON_NODE_TYPE_ARRAY:
	case VAR_JSON_NODE_TYPE_ROOT:
		for (i = 0; i < root->ec; i++) {
			if (!root->e[i]) continue;
			_node_free(root->e[i]);
		}
		if (root->e) free(root->e);
		break;
	case VAR_JSON_NODE_TYPE_STRING:
		free(root->wstring);
		break;
	case VAR_JSON_NODE_TYPE_INT:
		break;
	case VAR_JSON_NODE_TYPE_NUMBER:
		break;
	case VAR_JSON_NODE_TYPE_BOOL:
		break;
	case VAR_JSON_NODE_TYPE_NULL:
		break;
	}
	if (root->parent) {
		root->parent->e[root->index] = NULL;
	}
	free(root);
}


/******************************************************************************/
var_json_t *var_json_new()
{
	var_json_t *node = NULL;

	var_json_init();

	lock_write(&json_lock);
	node = _node_new(NULL, VAR_JSON_NODE_TYPE_ROOT, NULL);
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_object(var_json_t *root, const char *name)
{
	var_json_t *node = NULL;

	lock_write(&json_lock);
	node = _node_new(root, VAR_JSON_NODE_TYPE_OBJECT, name);
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_array(var_json_t *root, const char *name)
{
	var_json_t *node = NULL;

	lock_write(&json_lock);
	node = _node_new(root, VAR_JSON_NODE_TYPE_ARRAY, name);
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_str(var_json_t *root, const char *name, const char *value, ...)
{
	int err = 0;
	int size;
	va_list args;
	char *string = NULL;

	lock_write(&json_lock);

	var_json_t *node = _node_new(root, VAR_JSON_NODE_TYPE_STRING, name);

	/* format given string */
	if (node->wstring) {
		free(node->wstring);
		node->wstring = NULL;
	}
	va_start(args, value);
	size = vasprintf(&string, value, args);
	if (json_ambstowcs(&node->wstring, string) < 0) {
		node->wstring = wcsdup(L"<invalid string, conversion error>");
	}
	free(string);
	va_end(args);

	lock_unlock(&json_lock);

out_err:
	return node;
}


/******************************************************************************/
var_json_t *var_json_int(var_json_t *root, const char *name, int value)
{
	int err = 0;

	lock_write(&json_lock);
	var_json_t *node = _node_new(root, VAR_JSON_NODE_TYPE_INT, name);
	node->value = value;
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_number(var_json_t *root, const char *name, double value)
{
	int err = 0;

	lock_write(&json_lock);
	var_json_t *node = _node_new(root, VAR_JSON_NODE_TYPE_NUMBER, name);
	node->number = value;
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_boolean(var_json_t *root, const char *name, int value)
{
	int err = 0;

	lock_write(&json_lock);
	var_json_t *node = _node_new(root, VAR_JSON_NODE_TYPE_BOOL, name);
	node->value = value ? 1 : 0;
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_null(var_json_t *root, const char *name)
{
	int err = 0;
	var_json_t *node = NULL;

	lock_write(&json_lock);
	node = _node_new(root, VAR_JSON_NODE_TYPE_NULL, name);
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
var_json_t *var_json_find_by_key(var_json_t *root, char *name, int recursive)
{
	var_json_t *node = NULL;
	wchar_t *wname = NULL;

	lock_write(&json_lock);
	json_ambstowcs(&wname, name);
	node = _node_find_by_key(root, wname, recursive);
	free(wname);
	lock_unlock(&json_lock);

	return node;
}


/******************************************************************************/
char *var_json_to_str(var_json_t *root, size_t *len_ret)
{
	char *json_str = NULL;

	lock_write(&json_lock);

	size_t len = _node_to_str(root, NULL);
	json_str = malloc(len + 16);
	memset(json_str, 0, len + 16);
	_node_to_str(root, json_str);
	if (len_ret) *len_ret = len;

	lock_unlock(&json_lock);

	return json_str;
}


/******************************************************************************/
var_json_t *var_json_copy(var_json_t *root, var_json_t *to_copy)
{
	lock_write(&json_lock);
	var_json_t *ret = _node_clone(to_copy, root);
	lock_unlock(&json_lock);
	return ret;
}


/******************************************************************************/
void var_json_free(var_json_t *root)
{
	lock_write(&json_lock);
	_node_free(root);
	lock_unlock(&json_lock);
}


/******************************************************************************/
struct var_json_parse_stack *_parse_stack_new(int type, var_json_t *node)
{
	struct var_json_parse_stack *stack;
	stack = (struct var_json_parse_stack *)malloc(sizeof(*stack));
	memset(stack, 0, sizeof(*stack));
	stack->type = type;
	stack->node = node;
	return stack;
}


/******************************************************************************/
var_json_t *_parse_new(var_json_t *root)
{
	if (root == NULL) root = var_json_new();
	root->parse_bytes = 0;
	root->parse_stack = _parse_stack_new(JSON_PARSE_START, root);
	root->parse_stack->node = root;
	return root;
}


/******************************************************************************/
var_json_t *var_json_new_parser(var_json_t *root)
{
	ssize_t err = 0;
	lock_write(&json_lock);
	root = _parse_new(root);
	lock_unlock(&json_lock);
	return root;
}


#define IF_NODEEND(value) do { if (value) { node_end = 1; goto out_err; } } while(0)
#define IF_ERS(errval, retval, _s_err) \
do { \
	if ((errval) != 0) { \
		errstr = _s_err; \
		err = retval; \
		goto out_err; \
	} \
} while (0)

/******************************************************************************/
ssize_t var_json_parse_byte(var_json_t *root, int byte)
{
	const char *s_true = "true", *s_false = "false", *s_null = "null";
	int err = 0, node_end = 0;
	char *errstr = NULL;
	struct var_json_parse_stack *stack = root->parse_stack;
	int type = stack->type, special = stack->special;
	char *data = stack->data;
	size_t len = stack->len, size = stack->size;
	wchar_t uxxxx = stack->uxxxx;
	var_json_t *node = stack->node;

	/* check if end of string is received before it should be */
	IF_ERS(type != JSON_PARSE_END && type != JSON_PARSE_START && type != JSON_PARSE_NUMBER && byte == '\0', -1, "end of data reached too early");
	/* if end of string reached */
	IF_ER(byte == '\0', 0);
	/* if not parsing string, skip spaces */
	IF_ER(type != JSON_PARSE_STRING && isspace(byte), 0);
	/* dont accept any other character that whitespace after end of json */
	IF_ERS(type == JSON_PARSE_END, -1, "end of json found but data still has other characters than whitespace");

	/* if string */
	if (type == JSON_PARSE_STRING || type == JSON_PARSE_KEY) {
		IF_NODEEND(!special && byte == '\"');

		/* if special char */
		if (byte == '\\' && special == 0) special = 1;
		else if (byte != '\0') {
			int nchars = 1;
			char s_uxxxx[16];

			/* convert special chars into their right forms
			 * (skip ", \ and / since they are already in their right form)
			 */
			/** @todo handle 4hex digit form parsing */
			if (special == 1) {
				switch (byte) {
				case 'b':
					byte = '\b';
					break;
				case 'f':
					byte = '\f';
					break;
				case 'n':
					byte = '\n';
					break;
				case 'r':
					byte = '\r';
					break;
				case 't':
					byte = '\t';
					break;
				case 'u':
					uxxxx = 0;
					special = 2;
					OUT(0);
				case '\"':
				case '\\':
				case '/':
					break;

				default:
					IF_ERS(1, -1, "invalid escaped character");
				}
			}
			/* if parsing uXXXX character */
			else if (special >= 2) {
				mbstate_t mbs;
				int hex = tolower(byte);

				/* accept hex only */
				IF_ERS(!isxdigit(hex), -1, "non-hexdigit in backslashed unicode character");
				/* convert char into its number form */
				if (hex >= '0' && hex <= '9') hex -= '0';
				else hex += -'a' + 0xa;
				/* count byte offset */
				int e = (3 - (special - 2)) * 4;
				uxxxx |= (hex << e);
				special++;
				if (special < 6) OUT(0);

				memset(&mbs, 0, sizeof(mbs));
				memset(s_uxxxx, 0, sizeof(s_uxxxx));
				nchars = wcrtomb(s_uxxxx, uxxxx, &mbs);
			}

			if ((len + nchars + 1) >= size) {
				size += 256;
				data = realloc(data, size);
			}

			if (special == 6) {
				strcpy(&data[len], s_uxxxx);
				len += strlen(s_uxxxx);
			} else {
				data[len] = (char)byte;
				len++;
				data[len] = '\0';
			}
			special = 0;
		}
	}

	if (type == JSON_PARSE_SOMETHING ||
	        type == JSON_PARSE_START ||
	        type == JSON_PARSE_OBJECT ||
	        type == JSON_PARSE_ARRAY) {
		var_json_t *this = node;
		struct var_json_parse_stack *stack_top = NULL;

		if (type == JSON_PARSE_OBJECT) {
			if (byte == '\"' && stack->key == NULL) type = JSON_PARSE_KEY;
			else if (byte == ':') {
				if (stack->key == NULL) IF_ERS(1, -1, "parsing object item without key");
				type = JSON_PARSE_SOMETHING;
			} else if (stack->key != NULL) IF_ERS(1, -1, "key already parsed");
			else if (byte == '}') IF_NODEEND(1);
			else if (byte == ',') OUT(0);
			else IF_ERS(1, -1, "error parsing object");
		} else if (byte == '\"') type = JSON_PARSE_STRING;
		else if (isdigit(byte) || byte == '-') type = JSON_PARSE_NUMBER;
		else if (byte == 't') type = JSON_PARSE_TRUE;
		else if (byte == 'f') type = JSON_PARSE_FALSE;
		else if (byte == 'n') type = JSON_PARSE_NULL;
		else if (byte == '{') {
			type = JSON_PARSE_OBJECT;
			this = var_json_object(node, NULL);
		} else if (byte == '[') {
			type = JSON_PARSE_ARRAY;
			this = var_json_array(node, NULL);
		} else if (byte == ']' && type == JSON_PARSE_ARRAY) IF_NODEEND(1);
		else if (byte == ',' && type == JSON_PARSE_ARRAY) OUT(0);
		else IF_ERS(1, -1, "error parsing json");

		if (stack->type == JSON_PARSE_START) stack->type = JSON_PARSE_END;
		stack_top = _parse_stack_new(type, this);
		stack_top->prev = stack;
		root->parse_stack = stack = stack_top;
		data = NULL;
		len = 0;
		size = 0;
		special = 0;
		uxxxx = 0;
	}

	if (type == JSON_PARSE_NUMBER) {
		int digit = tolower(byte);
		if (special == 0 && (isdigit(digit) || digit == '-'));
		else if (special == 1 && (isdigit(digit) || digit == '.' || digit == 'e'));
		else if (special == 2 && (isdigit(digit) || digit == 'e'));
		else if (special == 3 && (isdigit(digit) || digit == '-') || digit == '+');
		else if (special == 4 && (isdigit(digit)));
		else IF_NODEEND(1);

		if (special == 0) special = 1;
		else if (special == 1 && digit == '.') special = 2;
		else if (special == 1 && digit == 'e') special = 3;
		else if (special == 2 && digit == 'e') special = 3;
		else if (special == 3) special = 4;


		if ((len + 2) >= size) {
			size += 256;
			data = realloc(data, size);
		}
		data[len] = (char)digit;
		len++;
		data[len] = '\0';
	}

	switch (type) {
	case JSON_PARSE_TRUE:
		IF_ERS(byte != (int)s_true[special], -1, "true misspelled");
		special++;
		IF_NODEEND(special >= 4);
		break;
	case JSON_PARSE_FALSE:
		IF_ERS(byte != (int)s_false[special], -1, "false misspelled");
		special++;
		IF_NODEEND(special >= 5);
		break;
	case JSON_PARSE_NULL:
		IF_ERS(byte != (int)s_null[special], -1, "null misspelled");
		special++;
		IF_NODEEND(special >= 4);
		break;
	}

out_err:
	if (byte == '\0' && type == JSON_PARSE_NUMBER) node_end = 1;
	root->parse_bytes++;
	if (!node_end) {
		stack->type = type;
		stack->data = data;
		stack->len = len;
		stack->size = size;
		stack->special = special;
		stack->uxxxx = uxxxx;
	} else {
		int dont_free_key = 0;
		double number;
		var_json_t *child = NULL;
		if (stack->prev) {
			child = stack->node;
			root->parse_stack = stack->prev;
			free(stack);
			stack = root->parse_stack;
			if (stack->type == JSON_PARSE_SOMETHING) {
				root->parse_stack = stack->prev;
				free(stack);
				stack = root->parse_stack;
			}
			if (stack->type != JSON_PARSE_OBJECT) child = NULL;
		} else stack->type = JSON_PARSE_END;
		switch (type) {
		case JSON_PARSE_STRING:
			var_json_str(stack->node, stack->key, "%s", data);
			break;
		case JSON_PARSE_KEY:
			stack->key = data;
			data = NULL;
			dont_free_key = 1;
			break;
		case JSON_PARSE_TRUE:
			var_json_boolean(stack->node, stack->key, 1);
			break;
		case JSON_PARSE_FALSE:
			var_json_boolean(stack->node, stack->key, 0);
			break;
		case JSON_PARSE_NULL:
			var_json_null(stack->node, stack->key);
			break;
		case JSON_PARSE_NUMBER:
			number = 0;
			if (sscanf(data, "%lf", &number) != 1) {
				err = -1;
				errstr = "invalid number format";
			} else {
				var_json_number(stack->node, stack->key, number);
			}
			break;
		case JSON_PARSE_ARRAY:
		case JSON_PARSE_OBJECT:
			if (stack->type == JSON_PARSE_OBJECT) {
				if (!stack->key) {
					err = -1;
					errstr = "object item missing key";
				} else {
					json_ambstowcs(&child->wname, stack->key);
					free(stack->key);
					stack->key = NULL;
				}
			}
			break;
		}
		if (stack->key != NULL && !dont_free_key) {
			free(stack->key);
			stack->key = NULL;
		}
		if (data) free(data);
		if (type == JSON_PARSE_NUMBER) var_json_parse_byte(root, byte);
	}
	if (err < 0) {
		if (root->parse_stack->error) {
			free(root->parse_stack->error);
			root->parse_stack->error = NULL;
		}
		asprintf(&root->parse_stack->error, "%s (bytes parsed %ld, character code at error 0x%02x)", errstr, root->parse_bytes, byte);
	}
	return err;
}


/******************************************************************************/
ssize_t var_json_parse_str(var_json_t *root, char *str)
{
	ssize_t err = 0;

	for ( ; (*str) != '\0'; str++) {
		err = var_json_parse_byte(root, (int)(*str));
		if (err != 0) return -(root->parse_bytes);
	}
	err = var_json_parse_byte(root, '\0');
	if (err != 0) return -(root->parse_bytes);

	return root->parse_bytes;
}


/******************************************************************************/
ssize_t var_json_parse_file(var_json_t *root, char *filename)
{
	ssize_t err = 0;
	FILE *f;
	int ch;

	f = fopen(filename, "r");
	if (!f) {
		asprintf(&root->parse_stack->error, "open file failed: %s", strerror(errno));
		return -1;
	}

	while ((ch = getc(f)) != EOF) {
		err = var_json_parse_byte(root, ch);
		if (err != 0) {
			fclose(f);
			return -(root->parse_bytes);
		}
	}
	fclose(f);
	err = var_json_parse_byte(root, '\0');
	if (err != 0) return -(root->parse_bytes);

	return root->parse_bytes;
}


/******************************************************************************/
char *var_json_get_key(var_json_t *node)
{
	size_t len = 0;
	mbstate_t mbs;
	const wchar_t *wname = node->wname;
	char *name;

	if (!wname) return NULL;
	memset(&mbs, 0, sizeof(mbs));
	len = wcsrtombs(NULL, &wname, 0, &mbs);
	if (len < 0) return NULL;
	name = malloc(len + 1);
	memset(name, 0, len + 1);
	memset(&mbs, 0, sizeof(mbs));
	wcsrtombs(name, &wname, len, &mbs);

	return name;
}


/******************************************************************************/
char *var_json_get_str(var_json_t *node)
{
	size_t len = 0;
	mbstate_t mbs;
	char *string;
	const wchar_t *wstring = node->wstring;

	if (!wstring || node->type != VAR_JSON_NODE_TYPE_STRING) {
		return NULL;
	}

	memset(&mbs, 0, sizeof(mbs));
	len = wcsrtombs(NULL, &wstring, 0, &mbs);
	if (len < 0) {
		return NULL;
	}

	string = malloc(len + 1);
	memset(string, 0, len + 1);
	memset(&mbs, 0, sizeof(mbs));
	wcsrtombs(string, &wstring, len, &mbs);

	return string;
}


/******************************************************************************/
int var_json_get_int(var_json_t *node)
{
	if (node->type == VAR_JSON_NODE_TYPE_INT) {
		return node->value;
	} else if (node->type == VAR_JSON_NODE_TYPE_NUMBER) {
		return (int)node->number;
	} else if (node->type == VAR_JSON_NODE_TYPE_STRING) {
		char *data = var_json_get_str(node);
		int value = atoi(data);
		free(data);
		return value;
	}
	return 0;
}


/******************************************************************************/
double var_json_get_number(var_json_t *node)
{
	if (node->type == VAR_JSON_NODE_TYPE_NUMBER) {
		return node->number;
	} else if (node->type == VAR_JSON_NODE_TYPE_INT) {
		return (double)node->value;
	} else if (node->type == VAR_JSON_NODE_TYPE_STRING) {
		char *data = var_json_get_str(node);
		double value = atof(data);
		free(data);
		return value;
	}
	return NAN;
}


/******************************************************************************/
int var_json_is_object(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_OBJECT) return 0;
	return 1;
}


/******************************************************************************/
int var_json_is_array(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_ARRAY) return 0;
	return 1;
}


/******************************************************************************/
int var_json_is_boolean(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_BOOL) return 0;
	return 1;
}


/******************************************************************************/
int var_json_is_null(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_NULL) return 0;
	return 1;
}


/******************************************************************************/
int var_json_is_true(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_BOOL) return 0;
	return node->value;
}


/******************************************************************************/
int var_json_is_false(var_json_t *node)
{
	if (node->type != VAR_JSON_NODE_TYPE_BOOL) return 0;
	return !node->value;
}


/******************************************************************************/
const char *var_json_parse_error(var_json_t *root)
{
	if (!root->parse_stack) return NULL;
	return root->parse_stack->error;
}


/******************************************************************************/
var_json_t **var_json_get_list(var_json_t *root, size_t *count)
{
	var_json_t **list = NULL;
	size_t i;

	if (root->type != VAR_JSON_NODE_TYPE_OBJECT && root->type != VAR_JSON_NODE_TYPE_ARRAY) return NULL;
	if (!count) return NULL;

	lock_write(&json_lock);
	(*count) = root->ec;
	list = (var_json_t **)malloc(sizeof(*list) * (*count));
	for (i = 0; i < (*count); i++) {
		list[i] = _node_clone(root->e[i], NULL);
	}
	lock_unlock(&json_lock);

out_err:
	return list;
}


/******************************************************************************/
void var_json_free_list(var_json_t **list, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		_node_free(list[i]);
	}
	free(list);
}


/******************************************************************************/
var_json_t *var_json_first_child(var_json_t *root)
{
	if (root->ec < 1) return NULL;
	return root->e[0];
}


