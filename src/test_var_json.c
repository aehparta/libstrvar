#include <stdio.h>
#include <locale.h>
#include <time.h>

#include "strjson.h"


int main(void)
{
	int i;
	var_json_t *json, *node, *node2, *node_a, *node3;
	ssize_t len, err;
	char *str, *x;

	setlocale(LC_ALL, "");
	
	json = var_json_new();
	
	/* test add/remove */
	var_json_str(json, NULL, "testi öitä jono juttu \t\n ääärh  決研県庫湖向幸港号根祭皿");
	str = var_json_to_str(json, &len);
	printf("%d\n%s\n", len, str);
	free(str);
	var_json_free(json);
/*	node_a = var_json_array(json, NULL);
	node = var_json_object(node_a, NULL);
	var_json_str(node, "var1", "jotain \"%s\"", "lisää");
	var_json_int(node, "name", 2324);
	var_json_str(node, "type", "message");
	
	str = var_json_to_str(json, &len);
	printf("%d\n%s\n", len, str);
	free(str);

	node = var_json_find_by_name(json, "name");
	var_json_str(node, NULL, "ter\rve\n");
	node = var_json_find_by_name(json, "type");
	var_json_null(node, NULL);

	node = var_json_find_by_name(json, "name");
	var_json_free(node);

	str = var_json_to_str(json, &len);
	printf("%d\n%s\n", len, str);
	free(str);
	
	node = var_json_find_by_name(json, "name2");
	printf("%p\n", node);

	node3 = var_json_new();
	node = var_json_object(node3, "test");
	node2 = var_json_copy(node, node_a);
	str = var_json_to_str(node3, &len);
	printf("%d\n%s\n", len, str);
	free(str);

	var_json_free(json);
	var_json_free(node3);
*/
	char *c, *s;
	const char *parse_tests[] =
	{
		"  \"false        moi as\"    ",
		"true",
		"false",
		"null",
		"999",
		"0.999",
		"-192.0",
		"-1e10",
		"0",
		"2e8",
		"\"\\\\x\\\\x\\/\"",
		"[]",
		"[      false        ]",
		"[ null, [], 3.141586, [ false, \"moi\" ] ]",
		"{ \"number\" : 98}",
		"{ \"广西平话廣西平話\":\"\\u5e7f\\u897f\\u5e73\\u8bdd\\u5ee3\\u897f\\u5e73\\u8a71\",\"muu\":\"moi\", \"harhar\": [ false, \"moi\" ]}",
		"[ \"moimoi terv hei \\\"\", null, false, \"jees\", 0, 100, 10.900293 ]",
		"{ \"unicode\" : \"广西平话廣西平話\" }",
		"",
		NULL
	};

//	len = var_json_parse_str(json, "  \"false        moi as\"    ");
//	len = var_json_parse_str(json, "true");
//  len = var_json_parse_str(json, "false");
	for (i = 0; parse_tests[i]; i++)
	{
		json = var_json_new_parser(NULL);
		err = var_json_parse_str(json, parse_tests[i]);
		str = var_json_to_str(json, &len);
		printf("%d, %d: \"%s\"\n", err, len, str);
		if (err < 0)
		{
			printf("err: \"%s\"\n", parse_tests[i]);
			printf("reason: %s\n", var_json_parse_error(json));
		}
		free(str);
		var_json_free(json);
	}

	json = var_json_new_parser(NULL);
	err = var_json_parse_file(json, "test1.json");
	str = var_json_to_str(json, &len);
	printf("%d, %d: \"%s\"\n", err, len, str);
	if (err < 0)
	{
		printf("err: \"%s\"\n", parse_tests[i]);
		printf("reason: %s\n", var_json_parse_error(json));
	}
	free(str);
	var_json_free(json);


	json = var_json_new_parser(NULL);
	err = var_json_parse_file(json, "test2.json");
	str = var_json_to_str(json, &len);
	printf("%d, %d: \"%s\"\n", err, len, str);
	if (err < 0)
	{
		printf("err: \"%s\"\n", parse_tests[i]);
		printf("reason: %s\n", var_json_parse_error(json));
	}
	free(str);
	var_json_free(json);

	printf("\n");

	json = var_json_new_parser(NULL);
	err = var_json_parse_file(json, "long.json");
	str = var_json_to_str(json, &len);
	printf("%d, %d: \"%s\"\n", err, len, str);
	free(str);
	var_json_free(json);

/*
	time_t start, end;
	char jsonstr1[16384], jsonstr2[16384];
	FILE *f;
	f = fopen("test1.json", "r");
	fread(jsonstr1, 1, 16384, f);
	fclose(f);
	f = fopen("test2.json", "r");
	fread(jsonstr2, 1, 16384, f);
	fclose(f);

	printf(" * performance test:\n");
	printf("parse: ");
	start = time(NULL);
	for (i = 0; i < 100000; i++)
	{
		json = var_json_new_parser(NULL);
		err = var_json_parse_str(json, jsonstr1);
		var_json_free(json);
		json = var_json_new_parser(NULL);
		err = var_json_parse_str(json, jsonstr2);
		var_json_free(json);
	}
	end = time(NULL);
	printf("%d seconds\n", end - start);*/
}

