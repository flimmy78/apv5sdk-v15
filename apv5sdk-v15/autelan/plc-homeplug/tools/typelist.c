/* ===================================================================*
 *
 *   void typelist (struct _type_ const list [], size_t size, char const * comma, FILE * fp);
 *
 *   symbol.h
 *   
 *   print a list of type names on the specified output stream;
 *   
 *.  Motley Tools by Charles Maier <cmaier@cmassoc.net>;
 *:  Published 2005 by Charles Maier Associates Limited;
 *;  Licensed under GNU General Public Licence Version 2 or later;
 *
 *--------------------------------------------------------------------*/

#ifndef TYPELIST_SOURCE
#define TYPELIST_SOURCE

#include <stdio.h>

#include "../tools/symbol.h"

void typelist (struct _type_ const list [], size_t size, char const * comma, char const * quote, FILE * fp) 

{
	struct _type_ const * item = list;
	if (list) while ((size_t)(item - list) < size) 
	{
		if (item > list) 
		{
			fputs (comma, fp);
		}
		if ((quote) && (*quote))
		{
			fputc (*quote++, fp);
		}
		fputs (item->name, fp);
		if ((quote) && (*quote))
		{
			fputc (*quote++, fp);
		}
		item++;
	}
	return;
}

#endif

