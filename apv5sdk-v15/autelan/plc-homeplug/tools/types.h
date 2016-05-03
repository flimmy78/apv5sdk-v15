/*====================================================================*
 *
 *   types.h - custom data type definitions and declarations; 
 *
 *   this file is a subset of the original that includes only those
 *   definitions and declaration needed for toolkit programs;
 *
 *.  Motley Tools by Charles Maier <cmaier@cmassoc.net>;
 *:  Published 2005 by Charles Maier Associates Limited;
 *;  Licensed under GNU General Public Licence Version 2 or later;
 *
 *--------------------------------------------------------------------*/

#ifndef TYPES_HEADER
#define TYPES_HEADER

/*====================================================================*
 *   system header files; 
 *--------------------------------------------------------------------*/

#include <stdint.h>

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

#if defined (WIN32)
#define SIZE_T_SPEC "%u"
#else
#define SIZE_T_SPEC "%zu"
#endif
                    
/*====================================================================*
 *   macros;
 *--------------------------------------------------------------------*/

#define SIZEOF(list) (sizeof(list)/sizeof(list[0]))

/*====================================================================*
 *   new POSIX types; 
 *--------------------------------------------------------------------*/

typedef signed errno_t;
typedef signed signo_t;
typedef unsigned char byte;

/*====================================================================*
 *   define C++ style true and false for use in standard C programs;
 *--------------------------------------------------------------------*/

#ifndef __cplusplus

typedef enum 

{
	false,
	true
}

bool;

#endif

/*====================================================================*
 *   cope with structure packing vagaries;
 *--------------------------------------------------------------------*/

#ifndef __packed
#ifdef __GNUC__
#define __packed __attribute__ ((packed))
#else
#define __packed
#endif
#endif

/*====================================================================*
 *   define flagword (bitmap) variable type for clarity;
 *--------------------------------------------------------------------*/

typedef signed file_t;
typedef signed sock_t;
typedef signed code_t;
typedef unsigned char byte_t;
typedef unsigned type_t;
typedef unsigned flag_t;
typedef struct _file_ 

{
	file_t file;
	char const *name;
}

file;
typedef struct _term_ 

{
	char const * term;
	char const * text;
}

TERM;
typedef struct _type_ 

{
	type_t type;
	char const * name;
}

TYPE;
typedef struct _code_ 

{
	code_t code;
	char const * name;
}

CODE;

/*====================================================================*
 *   end definitions and declarations;
 *--------------------------------------------------------------------*/

#endif

