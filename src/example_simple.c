/*
 * Simple example how to use libstrvar.
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/** @example example_simple.c
 * This is an example simple example how to use libstrvar.
 */


/******************************************************************************/
/* INCLUDES */
#include <strvar.h>
#include <strxml.h>
#include <ddebug/synchro.h>


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
/* Main. */
int main(int argc, char *argv[])
{
	double value;
	char *problem, *var_name;
	var_xml_t xml;

	if (argc != 2)
	{
		printf("Invalid arguments, usage: %s \"1+2+3...\"\n", argv[0]);
		return -1;
	}
	
	xml = var_xml_load(argv[1]);
	if (!xml)
	{
		printf("Failed to load given file!\n");
		return -1;
	}
	printf("\nDUMP: ---\n");
	var_xml_dump(stdout, xml);
	printf("\n--- :DUMP\n");
	var_xml_free(xml);
	return 0;

	problem = argv[1];

	/* Initialize variable library. */
	if (var_init() != 0) return -1;
	
	var_name = "variable";

	/*
	 * Set variable contents. Lets use default var list (0).
	 * It is always available.
	 */
	var_set_str(var_name, problem);
	
	/*
	 * Do things...
	 */
	
	/* Get variable value, and example calculate its value. */
	printf("variable value as ascii: \'%s\'\n", var_get_str(var_name));
//	printf("variable value as double: \'%lf\'\n", var_get_num(0, var_name));
//	printf("variable value as int: \'%d\'\n", var_get_int(0, var_name));
	/* Calculate value, gives invalid values fo not valid problems. */
	value = var_calc(var_name);
	printf("calculated value: \'%lf\'\n", value);

	/* Dump content of variables into stdout. */
	var_dump();
	
	/* Deinit library. */
	var_quit();

	return 0;
}

