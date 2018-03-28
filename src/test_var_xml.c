/*
 * libstrvar XML-library tests.
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
#include <strxml.h>


char *xml_mem = "<?xml version=\"1.0\"?>\
<test>\
    <content>This is a test XML file for libstrvar.</content>\
    <style font=\"Arial\" size=\"14\"/>\
</test>";

/******************************************************************************/
int main(int argc, char *argv[])
{
	var_xml_t xml;

	if (argc != 2)
	{
		printf("Invalid arguments, usage: %s <file>\n", argv[0], argv[1]);
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

/*    xml = var_xml_parse(xml_mem, strlen(xml_mem));
    printf("\nDUMP: ---\n");
    var_xml_dump(stdout, xml);
    printf("\n--- :DUMP\n");
    var_xml_free(xml);*/

	return 0;
}

