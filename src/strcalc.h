/*
 * Part of libstrvar.
 *
 * License: MIT, see LICENSE
 * Authors: Antti Partanen <aehparta@cc.hut.fi, duge at IRCnet>
 */

#ifndef _STRCALC_H
#define _STRCALC_H


/******************************************************************************/
/* INCLUDES */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


/******************************************************************************/
/* FUNCTION DEFINITIONS */
/**
 * @addtogroup strcalc
 * @{
 */
double strmath_calc(const char *, double (*)(const char *));
/** @} addtogroup strcalc */


/**
 * @addtogroup internal
 * @{
 */
int iscalcchar(int);
double calc_one_period(const char *, unsigned int *, unsigned int, double, double (*)(const char *));
/** @} addtogroup internal */


#endif /* _STRCALC_H */
/******************************************************************************/

