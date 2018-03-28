/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */


/******************************************************************************/
/* INCLUDES */
#include "main.h"


/******************************************************************************/
/* FUNCTIONS */
double get_var_val(const char *name)
{
	double ret = 1.0f;
	
	if (strcmp(name, "x") == 0) ret = 2.0f;
	else if (strcmp(name, "y") == 0) ret = 4.0f;
	else if (strcmp(name, "pi") == 0) ret = 3.141592f;
	
	return (ret);
}

/******************************************************************************/
/** Main. */
int main(int argc, char *argv[])
{
	int l[10], i, n, err, mode = 0, list_id;
	char *name, *str, str2[200];
	double val;
	hashl_t *list;
	
	
	
	exit(1);

	if (argc < 2) mode = 0xff;
	else if (argc > 1)
	{
		mode = atoi(argv[1]);
	}

	/* First test, simple test. */
	if (mode & 1)
	{
		list_id = 0;
		name = "variable";
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
		str = "contents of variable";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		name = "variable";
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
	}
	
	/* Second test, more complex. */
	if (mode & 2)
	{
		list_id = 0;
		name = "variable";
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
		str = "contents of variable in second test";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
		
		list_id = 0;
		name = "some-var-in-second-test";
		str = "contents of variable in second test, this is longer one than previous!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
	
		list_id = 1;
		name = "some-var-in-second-test";
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
		str = "contents of variable in second test";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(-1, name));
		
		list_id = var_list_new("new list");
		printf("got new list id %d\n", list_id);
		name = "some-var-in-second-test";
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(-1, name));
		str = "contents of variable in second test";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
		str = "1234567890";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
	
		str = "this should go to list everywhere";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(-1, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(-1, name));
	
		name = "short";
		val = -117.002783F;
		printf("set var \'%s\' to \'%lf\'\n", name, val);
		varl_set_num(list_id, name, val);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
	
		name = "10 bytes";
		str = "1234567890";
		printf("set var \'%s\' to \'%s\'\n", name, str);
		varl_set_str(list_id, name, "%s", str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(list_id, name));
	}

	/* Test parsing of variables content. */
	if (mode & 4)
	{
		name = "replace2";
		str = "variable contents";
		varl_set_str(0, name, str);
		name = "replace";
		str = "\"$replace2$!!!\"";
		varl_set_str(0, name, str);
		name = "calc";
		str = "Hello. $calc <- causes deadlock! $replace should not say $$replace, it should say $replace...";
		varl_set_str(0, name, str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(0, name));
		str = varl_parse(-1, name, 0, 1, 2, 3, 4, 5, -1);
		printf("as parsed it is '\%s\'\n", str);
		varl_free(str);
		str = varl_parse(-1, name, 0, 1, 2, 3, 4, 5, -1);
		printf("as parsed it is '\%s\'\n", str);
		var_free(str);
	}

	/* Test calculation. */
	if (mode & 8)
	{
		name = "math";
		str = "1 + 2 * 7 + (2 + 6) / 9 * sin(0.5)";
		varl_set_str(0, name, str);
		printf("find var \'%s\':\n result: \'%s\'\n", name, varl_get_str(0, name));
		str = varl_parse(0, name, 0, -1);
		printf("as parsed it is '\%s\'\n", str);
		var_free(str);
		val = varl_calc(0, name, 0, -1);
		printf("calculated as '\%lf\'\n", val);
	}
	
	/* Some test. */
	if (mode & 10)
	{
		var_list_new(NULL);
	}

	var_dump();
	var_quit();

	return (0);
}

