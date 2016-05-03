/*====================================================================*
 *
 *   timer.h - custom data type definitions and declarations; 
 *
 *   this file is a subset of the original that includes only those
 *   definitions and declaration needed for toolkit programs;
 *
 *.  Motley Tools by Charles Maier <cmaier@cmassoc.net>;
 *:  Published 2005 by Charles Maier Associates Limited;
 *;  Licensed under GNU General Public Licence Version 2 or later;
 *
 *--------------------------------------------------------------------*/

#ifndef TIMER_HEADER
#define TIMER_HEADER

/*====================================================================*
 *   system header files; 
 *--------------------------------------------------------------------*/

#include <stdint.h>

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

#ifdef WIN32
#define SLEEP(n) Sleep(n)
#else
#define SLEEP(n) usleep((n)*1000)
#endif

/*====================================================================*
 *   macros;
 *--------------------------------------------------------------------*/

#define SECONDS(start,timer) (((timer).tv_sec - (start).tv_sec) + ((timer).tv_usec - (start).tv_usec - 50000) / 1000000) 
#define MILLISECONDS(start,timer) ((((timer).tv_sec - (start).tv_sec) * 1000) + ((timer).tv_usec - (start).tv_usec) / 1000)

/*====================================================================*
 *   end definitions and declarations;
 *--------------------------------------------------------------------*/

#endif

