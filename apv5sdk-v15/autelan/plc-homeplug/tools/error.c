/*====================================================================*
 *
 *   void error (signed status, errno_t number, char const * format, ...);
 *
 *   error.h
 *
 *   custom implementation of GNU error() function for systems
 *   that do not have it;
 *
 *.  Motley Tools by Charles Maier <cmaier@cmassoc.net>;
 *:  Published 2005 by Charles Maier Associates Limited;
 *;  Licensed under GNU General Public Licence Version 2 or later;
 *
 *--------------------------------------------------------------------*/

#ifndef ERROR_SOURCE
#define ERROR_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../tools/types.h"
#include "../tools/error.h"

#ifdef __GNUC__

__attribute__ ((format (printf, 3, 4))) 

#endif

void error (signed status, errno_t number, char const * format, ...) 

{
	extern char const *program_name;
	if ((program_name) && (*program_name)) 
	{
		fprintf (stderr, "%s: ", program_name);
	}

#if 0

	if ((format) && (*format)) 
	{
		va_list arglist;
		va_start (arglist, format);
		vfprintf (stderr, format, arglist);
		va_end (arglist);
	}
	if (number) 
	{
		fprintf (stderr, ": %s", strerror (number));
	}

#else

	if (number) 
	{
		fprintf (stderr, "%s: ", strerror (number));
	}
	if ((format) && (*format)) 
	{
		va_list arglist;
		va_start (arglist, format);
		vfprintf (stderr, format, arglist);
		va_end (arglist);
	}

#endif

	fprintf (stderr, "\n");
	fflush (stderr);
	if (status) 
	{
		exit (status);
	}
	return;
}


#endif

