/*====================================================================*
 *
 *   struct node const * xmlvalue (struct node const * node);
 *
 *   node.h
 *
 *   search an attribute node for the value node; return the value
 *   node address;
 *
 *.  Motley Tools by Charles Maier <cmaier@cmassoc.net>;
 *:  Published 2001-2006 by Charles Maier Associates Limited;
 *;  Licensed under GNU General Public Licence Version 2 or later;
 *
 *--------------------------------------------------------------------*/

#ifndef XMLVALUE_SOURCE
#define XMLVALUE_SOURCE

#include <string.h>

#include "../nodes/node.h"

struct node const * xmlvalue (struct node const * node) 

{
	if (node) 
	{
		node = node->below;
	}
	while (node) 
	{
		if (node->type == NODE_VALU) 
		{
			break;
		}
		node = node->after;
	}
	return (node);
}


#endif

