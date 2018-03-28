#include <stdio.h>
#include "strhash.h"


int main(void)
{
	/* test add/remove */
	var_lh_t l = var_lh_new(2, NULL, NULL);

	var_lh_puta(l, "1", "a");
	var_lh_puta(l, "2", "2");
	var_lh_puta(l, "1", "1");
	var_lh_puta(l, "3", "3");
	var_lh_puta(l, "5", "5");
	var_lh_puta(l, "6", "6");
	var_lh_puta(l, "4", "4");

	var_lh_rm(l, "1");
	var_lh_rm(l, "2");
	var_lh_rm(l, "3");
	
	var_lh_dump(l);

	var_lh_rm(l, "1");
	var_lh_rm(l, "2");
	var_lh_rm(l, "3");

	var_lh_dump(l);

	var_lh_rm(l, "4");
	var_lh_rm(l, "5");
	var_lh_rm(l, "6");

	var_lh_dump(l);

	var_lh_free(l);

	l = var_lh_new(4, NULL, NULL);
	char *p1 = "moikka1";
	char *p2 = "moikka2";
	char *p3 = "moikka3";
	char *p4 = "moikka4";
	char *p5 = "moikka5";
	char *p;
	var_lh_setp(l, "1", p1);
	var_lh_setp(l, "2", p2);
	var_lh_setp(l, "3", p3);
	var_lh_setp(l, "4", p4);
	var_lh_setp(l, "5", p5);
	var_lh_dump(l);
	while (p = var_lh_popp(l))
	{
		printf("---- %s\n", p);
	}
	var_lh_dump(l);
	var_lh_free(l);
}

