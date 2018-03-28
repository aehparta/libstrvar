/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

/******************************************************************************/
/* INCLUDES */
#include "strcalc.h"


#ifdef _DEBUG
#define PRINTF printf
#else
#define PRINTF
#endif


/******************************************************************************/
/* DEFINES */
#define SIGN_PLUS	0
#define SIGN_MINUS	1

#define CALC_NONE	0

#define CALC_OR		20
#define CALC_AND	21
#define CALC_XOR	22
#define CALC_REM	23

#define CALC_SUB	50
#define CALC_ADD	51
#define CALC_MUL	52
#define CALC_DIV	53

#define CALC_POW	70
#define CALC_FAC	71

#define CALCS_NONE		0
#define CALCS_SUBFUNC	1


/******************************************************************************/
/* FUNCTIONS */

/******************************************************************************/
/**
 * Remove white spaces from string.
 */
void calc_strip_whitespaces(char *to, const char *from)
{
	for (; *from != '\0'; from++)
	{
		if (isspace(*from)) continue;
		*to++ = *from;
	}
	*to = '\0';
}


/******************************************************************************/
/**
 * Check whether char is some calculatory character or not.
 * (means plus, minus and other characters)
 *
 * @return Value defined by CALC_-macros.
 */
int iscalcchar(int ch)
{
	int ret = CALC_NONE;
	
	switch (ch)
	{
	case '*':
		ret = CALC_MUL;
		break;
	
	case '/':
		ret = CALC_DIV;
		break;
	
	case '+':
		ret = CALC_ADD;
		break;
	
	case '-':
		ret = CALC_SUB;
		break;
	
	case '|':
		ret = CALC_OR;
		break;
	
	case '&':
		ret = CALC_AND;
		break;
	
	case '^':
		ret = CALC_XOR;
		break;
	
	case '%':
		ret = CALC_REM;
		break;
	
	case '\"':
		ret = CALC_POW;
		break;
	
	case '!':
		ret = CALC_FAC;
		break;
	}
	
	PRINTF("char is %d\n", ret);

	return ret;
}


/******************************************************************************/
/**
 *
 */
double calc_add_to_with(double val, double ret, int calc)
{
	long long _ret, _val;
	
	_ret = (long long)ret;
	_val = (long long)val;

	switch (calc)
	{
	case CALC_NONE:
	case CALC_ADD:
		PRINTF("adding %lf to %lf", val, ret);
		ret += val;
		break;
	case CALC_SUB:
		PRINTF("subtracting %lf from %lf", val, ret);
		ret -= val;
		break;
	case CALC_MUL:
		PRINTF("multiplying %lf with %lf", ret, val);
		ret *= val;
		break;
	case CALC_DIV:
		PRINTF("dividing %lf with %lf", ret, val);
		ret /= val;
		break;
	case CALC_OR:
		PRINTF("or %lf with %lf", ret, val);
		ret = (double)(_ret | _val);
		break;
	case CALC_AND:
		PRINTF("and %lf with %lf", ret, val);
		ret = (double)(_ret & _val);
		break;
	case CALC_XOR:
		PRINTF("xor %lf with %lf", ret, val);
		ret = (double)(_ret ^ _val);
		break;
	case CALC_REM:
		PRINTF("reminder %lf with %lf", ret, val);
		ret = (double)(_ret % _val);
		break;
	case CALC_POW:
		PRINTF("power of %lf with %lf", ret, val);
		ret = (double)pow(_ret, _val);
		break;
	case CALC_FAC:
		PRINTF("factor of %lf", ret, val);
		//ret = (double)(_ret % _val);
		break;
	}
	
	PRINTF(" = %lf\n", ret);
	
	return ret;
}


/******************************************************************************/
/**
 *
 */
double calc_function(char *function, double val)
{
	/* Variables. */
	double ret = val;

	/* If there was no function at all. */
	if (strlen(function) < 1) ret = val;
	/* What is the function, find out. */
	else if (strcmp(function, "sin") == 0) ret = sin(val);
	else if (strcmp(function, "cos") == 0) ret = cos(val);
	else if (strcmp(function, "tan") == 0) ret = tan(val);
	
	PRINTF("function %s(%lf) returned value %lf\n", function, val, ret);
	
	return ret;
}


/******************************************************************************/
/**
 * Calculate given math problem and return result as double.
 * <br><b>Example:</b> "1 + 2  * (2 + 6) / 9*sin(0.5)" = 15.426156
 *
 * @param problem Mathematic problem to calculate.
 * @param varval This is pointer to function used for fetching unknown
 *               variable values. Variable name is given to the function
 *               and it should return value which the variable has.
 *               This can be NULL.
 * @return Result, no more, no less.
 */
double strmath_calc(const char *problem, double (*varval)(const char *))
{
	/* Variables. */
	double ret;
	unsigned int stop;
	char *nowhite;
	
	nowhite = malloc(strlen(problem) + 1);
	if (!nowhite) return (ret);
	
	calc_strip_whitespaces(nowhite, problem);
	PRINTF("whites: \'%s\', no whites: \'%s\'\n", problem, nowhite);
	ret = calc_one_period(nowhite, &stop, CALC_NONE, 0.0f, varval);
	
	free(nowhite);
	
	return ret;
}


/******************************************************************************/
/**
 * Calculate until end of string or closing bracket found or lower priority
 * calculation found (priority means that example multiplication has higher
 * priority than sum).
 */
double calc_one_period(const char *problem, unsigned int *stop,
                       unsigned int calc, double ret,
                       double (*varval)(const char *))
{
	/* Variables. */
	unsigned int sign = SIGN_PLUS, state = CALCS_NONE;
	unsigned int i, n;
	char *pch, *br, var_name[100];
	double val;
	
	for (pch = (char *)problem; *pch != '\0'; pch++)
	{
		/* Check for end condition. */
		PRINTF("char is now %d / %c\n", (int)*pch, *pch);
		if (*pch == '\0')
		{
			PRINTF("break EOS\n");
			break;
		}
		
		i = iscalcchar(*pch);
		if (i == CALC_NONE)
		{
			PRINTF("was not (%c)\n", *pch);
		}
		else if (i < calc)
		{
			PRINTF("break i < calc\n");
			break;
		}
		else
		{
			PRINTF("current %d\n", i);
			calc = i;
			continue;
		}
		
		/* Find end of current value. */
		br = strpbrk(pch, "+-*/|&^%()!\"");
		/*
			If end of string found, set br pointing to null-char.
			(br must not be null after this, it is used!)
		*/
		if (!br) br = &pch[strlen(pch)];
		memset(var_name, 0, 100);
		memcpy(var_name, pch, (unsigned int)(br - pch));
		
		/* If next is a calculation in brackets. */
		if (*br == '(')
		{
			PRINTF("entering subfunction 1: %s\n", br);
			br++;
			val = calc_one_period(br, &n, CALC_NONE, 0.0f, varval);
			br += n;
			pch = br - 1;
			PRINTF("subfunction end 1, left %s (used %d chars)\n", pch, n);
			
			/*
				Check whether this was not only brackets,
				but some function like sin() or cos().
			*/
			val = calc_function(var_name, val);
			
			/*
				Check if following calculation has higher
				priority than previous before brackets.
			*/
			if (*pch == ')') pch++;
			while (1)
			{
				i = iscalcchar(*pch);
				if (i > calc)
				{
					PRINTF("entering subfunction 2: %s\n", pch);
					val = calc_one_period(pch, &n, i, val, varval);
					PRINTF("subfunction end 2, left %s (used %d chars)\n", pch, n);
					pch += n;
					PRINTF("subfunction end 2, left %s (used %d chars)\n", pch, n);
				}
				else break;
			}
			ret = calc_add_to_with(val, ret, calc);
			if (*pch == 0) pch--;
			continue;
		}
		else
		{
			PRINTF("break by %c\n", *br);
			i = iscalcchar(*br);
			if (i > calc && calc > CALC_NONE)
			{
				PRINTF("entering subfunction 3: %s\n", pch);
				val = calc_one_period(pch, &n, CALC_NONE, 0.0f, varval);
				pch += n - 1;
				PRINTF("subfunction end 3, left %s (used %d chars)\n", pch, n);
				ret = calc_add_to_with(val, ret, calc);
				continue;
			}
			else if (i > calc) state = CALCS_SUBFUNC;
		}
		
		PRINTF("found value %s\n", var_name);
		n = sscanf(var_name, "%lf", &val);
		
		if (n != 1 && varval) val = varval(var_name);
		else if (n != 1) val = 1.0f;
		
		if (state == CALCS_SUBFUNC)
		{
			PRINTF("entering subfunction 4: %s\n", br);
			br++;
			val = calc_one_period(br, &n, i, val, varval);
			br += n - 1;
			pch = br;
			PRINTF("subfunction end 4, left %s (used %d chars)\n", pch, n);
			state = CALCS_NONE;
		}
		
		ret = calc_add_to_with(val, ret, calc);

		PRINTF("value is now %lf\n", ret);
		
		/* If subfunction ended. */
		if (*br == ')')
		{
			PRINTF("found closing bracket (%p, %p)\n", pch, br);
			pch = (br + 1);
			break;
		}
		/* If end of string found. */
		if (*br == '\0')
		{
			PRINTF("end of string\n");
			pch = br;
			break;
		}
	}
	
	PRINTF("pointers are: %p, %p, %p\n", problem, pch, br);
	
	*stop = (unsigned int)(pch - problem);
	
	/* Return. */
	return ret;
}

