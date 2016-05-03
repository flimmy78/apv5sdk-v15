/*====================================================================*
 *   
 *   Copyright (c) 2011 by Qualcomm Atheros
 *   
 *   Permission to use, copy, modify, and/or distribute this software 
 *   for any purpose with or without fee is hereby granted, provided 
 *   that the above copyright notice and this permission notice appear 
 *   in all copies.
 *   
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL  
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *   
 *--------------------------------------------------------------------*/

#define _GETOPT_H

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../tools/getoptv.h"
#include "../tools/memory.h"
#include "../tools/flags.h"
#include "../tools/error.h"
#include "../tools/files.h"
#include "../ram/sdram.h"
#include "../nvm/nvm.h"

/*====================================================================*
 *   custom source files;
 *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/getoptv.c"
#include "../tools/putoptv.c"
#include "../tools/version.c"
#include "../tools/checksum32.c"
#include "../tools/fdchecksum32.c"
#include "../tools/strfbits.c"
#include "../tools/error.c"
#endif

#ifndef MAKEFILE
#include "../ram/sdrampeek.c"
#endif

#ifndef MAKEFILE
#include "../nvm/nvm.c"
#include "../nvm/nvmpeek.c"
#include "../nvm/nvmpeek1.c"
#include "../nvm/nvmpeek2.c"
#include "../nvm/manifest.c"
#endif

/*====================================================================*
 *   program constants;
 *--------------------------------------------------------------------*/

#define HARDWARE 0
#define SOFTWARE 1
#define VER 2
#define REV 3
#define BUILD 6
#define DATE 7

/*====================================================================*
 *
 *   unsigned string2vector (char ** vector, length, char * string, char c);
 *
 *   convert string to a vector and return vector count; split string
 *   on characer (c);
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static unsigned string2vector (char ** vector, char * string, char c) 

{
	char ** origin = vector;
	for (*vector++ = string; *string; string++) 
	{
		if (*string == c) 
		{
			*string++ = (char)(0);
			*vector++ = string--;
		}
	}
	*vector = (char *)(0);
	return ((unsigned)(vector - origin));
}


/*====================================================================*
 *
 *   void firmware (struct _file_ * file, unsigned offset, signed module, flag_t flags);
 *
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static void firmware (char const * filename, signed fd, unsigned module, unsigned offset, flag_t flags) 

{
	char memory [512];
	read (fd, memory, sizeof (memory));
	lseek (fd, (off_t)(0) - sizeof (memory), SEEK_CUR);
	if (_anyset (flags, NVM_FIRMWARE)) 
	{
		if ((*memory > 0x20) && (*memory < 0x7F)) 
		{
			printf ("%s (%d) %s\n", filename, module, memory);
		}
		else 
		{
			printf ("%s (%d) %s\n", filename, module, memory + offset);
		}
	}
	if (_anyset (flags, NVM_IDENTITY)) 
	{
		char * vector [16];
		char buffer [256];
		if ((* memory > 0x20) && (* memory < 0x7f)) 
		{
			strncpy (buffer, memory, sizeof (buffer));
		}
		else 
		{
			strncpy (buffer, memory + offset, sizeof (buffer));
		}
		string2vector (vector, buffer, '-');
		printf ("%s ", vector [HARDWARE]);
		printf ("%04d ", atoi (vector [BUILD]));
		printf ("%s ", vector [DATE]);
		printf ("%s.", vector [VER]);
		printf ("%s ", vector [REV]);
		printf ("%s (%d)\n", filename, module);
		return;
	}
	return;
}


/*====================================================================*
 *
 *   signed nvmimage (char const * filename, flag_t flags);
 *
 *   validate a PLC firmware module file; keep as little information 
 *   as possible in memory; this slows validation but minimizes the 
 *   resources needed at runtime; resource requirements do not grow
 *   with file size;
 *
 *   open an NVM file and validate it by walking headers and modules
 *   to verify lengths and checksums; return 0 on success or -1 on
 *   error;
 *
 *   the checksum of the entire header, including header checksum, is
 *   always 0 for valid headers; similarly, the checksum of the module
 *   and module checksum is always 0 for valid modules;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed nvmimage (char const * filename, flag_t flags) 

{
	struct nvm_header1 nvm_header;
	struct config_ram config_ram;
	signed module = 0;
	signed fd;
	memset (&nvm_header, 0, sizeof (nvm_header));
	if ((fd = open (filename, O_BINARY|O_RDONLY)) == -1) 
	{
		if (_allclr (flags, NVM_SILENCE)) 
		{
			error (0, errno, FILE_CANTOPEN, filename);
		}
		return (-1);
	}
	do 
	{
		if (read (fd, &nvm_header, sizeof (nvm_header)) != sizeof (nvm_header)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_CANTREAD, filename, module);
			}
			return (-1);
		}
		if (LE32TOH (nvm_header.HEADERVERSION) != 0x60000000) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_VERSION, filename, module);
			}
			return (-1);
		}
		if (checksum32 (&nvm_header, sizeof (nvm_header), 0)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_CHECKSUM, filename, module);
			}
			return (-1);
		}
		if (_anyset (flags, NVM_VERBOSE)) 
		{
			printf ("------- %s (%d) -------\n", filename, module);
			nvmpeek1 (&nvm_header);
		}
		if (nvm_header.HEADERMINORVERSION) 
		{
			if (LE32TOH (nvm_header.IMAGETYPE) == NVM_IMAGE_CONFIG_SYNOPSIS) 
			{
				if (_anyset (flags, NVM_SDRAM)) 
				{
					printf ("------- %s (%d) -------\n", filename, module);
					read (fd, &config_ram, sizeof (config_ram));
					lseek (fd, (off_t)(0) - sizeof (config_ram), SEEK_CUR);
					sdrampeek (&config_ram);
				}
			}
			else if (LE32TOH (nvm_header.IMAGETYPE) == NVM_IMAGE_FIRMWARE) 
			{
				firmware (filename, fd, module, 0x70, flags);
			}
		}
		else if (!module) 
		{
			if (_anyset (flags, NVM_SDRAM)) 
			{
				printf ("------- %s (%d) -------\n", filename, module);
				read (fd, &config_ram, sizeof (config_ram));
				lseek (fd, (off_t)(0) - sizeof (config_ram), SEEK_CUR);
				sdrampeek (&config_ram);
			}
		}
		else if (!nvm_header.NEXTHEADER) 
		{
			firmware (filename, fd, module, 0x70, flags);
		}
		if (fdchecksum32 (fd, LE32TOH (nvm_header.IMAGELENGTH), nvm_header.IMAGECHECKSUM)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_IMG_CHECKSUM, filename, module);
			}
			return (-1);
		}
		module++;
	}
	while (nvm_header.NEXTHEADER);
	if (lseek (fd, 0, SEEK_CUR) != lseek (fd, 0, SEEK_END)) 
	{
		if (_allclr (flags, NVM_SILENCE)) 
		{
			error (0, errno, NVM_HDR_LINK, filename, module);
		}
		return (-1);
	}
	close (fd);
	return (0);
}


/*====================================================================*
 *
 *   signed nvmchain (char const * filename, flag_t flags);
 *
 *   validate a PLC firmware module file; keep as little information 
 *   as possible in memory; this slows validation but minimizes the 
 *   resources needed at runtime; resource requirements do not grow
 *   with file size;
 *
 *   open an NVM file and validate it by walking headers and modules
 *   to verify lengths and checksums; return 0 on success or -1 on
 *   error;
 *
 *   the checksum of the entire header, including header checksum, is
 *   always 0 for valid headers; similarly, the checksum of the module
 *   and module checksum is always 0 for valid modules;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed nvmchain (char const * filename, flag_t flags) 

{
	struct nvm_header2 nvm_header;
	unsigned module = 0;
	size_t origin = ~0;
	size_t offset = 0;
	signed fd;
	if ((fd = open (filename, O_BINARY|O_RDONLY)) == -1) 
	{
		if (_allclr (flags, NVM_SILENCE)) 
		{
			error (0, errno, FILE_CANTOPEN, filename);
		}
		return (-1);
	}
	do 
	{
		if (read (fd, &nvm_header, sizeof (nvm_header)) != sizeof (nvm_header)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_CANTREAD, filename, module);
			}
			return (-1);
		}
		if (LE16TOH (nvm_header.MajorVersion) != 1) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_VERSION, filename, module);
			}
			return (-1);
		}
		if (LE16TOH (nvm_header.MinorVersion) != 1) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_VERSION, filename, module);
			}
			return (-1);
		}
		if (checksum32 (&nvm_header, sizeof (nvm_header), 0)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_CHECKSUM, filename, module);
			}
			return (-1);
		}
		if (LE32TOH (nvm_header.PrevHeader) != origin) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_HDR_LINK, filename, module);
			}
			return (-1);
		}
		if (_anyset (flags, NVM_VERBOSE)) 
		{
			printf ("------- %s (%d) -------\n", filename, module);
			nvmpeek2 (&nvm_header);
		}
		if (LE32TOH (nvm_header.ImageType) == NVM_IMAGE_MANIFEST) 
		{
			if (_anyset (flags, NVM_MANIFEST)) 
			{
				uint32_t extent = LE32TOH (nvm_header.ImageLength);
				byte * memory = malloc (extent);
				if (!memory) 
				{
					error (1, errno, FILE_CANTLOAD, filename);
				}
				if (read (fd, memory, extent) != (signed)(extent)) 
				{
					error (1, errno, FILE_CANTREAD, filename);
				}
				if (lseek (fd, (off_t)(0)-extent, SEEK_CUR) == -1) 
				{
					error (1, errno, FILE_CANTHOME, filename);
				}
				printf ("------- %s (%d) -------\n", filename, module);
				if (manifest (memory, extent)) 
				{
					error (1, errno, "Bad manifest: %s", filename);
				}
				free (memory);
			}
		}
		if (fdchecksum32 (fd, LE32TOH (nvm_header.ImageLength), nvm_header.ImageChecksum)) 
		{
			if (_allclr (flags, NVM_SILENCE)) 
			{
				error (0, errno, NVM_IMG_CHECKSUM, filename, module);
			}
			return (-1);
		}
		origin = offset;
		offset = LE32TOH (nvm_header.NextHeader);
		module++;
	}
	while (~nvm_header.NextHeader);
	if (lseek (fd, 0, SEEK_CUR) != lseek (fd, 0, SEEK_END)) 
	{
		if (_allclr (flags, NVM_SILENCE)) 
		{
			error (0, errno, NVM_HDR_LINK, filename, module);
		}
		return (-1);
	}
	close (fd);
	return (0);
}


/*====================================================================*
 *
 *   signed chknvm (char const * filename, flag_t flags);
 *
 *   validate a PLC firmware module file; keep as little information 
 *   as possible in memory; this slows validation but minimizes the 
 *   resources needed at runtime; resource requirements do not grow
 *   with file size;
 *
 *   determine file format based on module header version then rewind
 *   the file and call the appropriate chknvm; 
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed chknvm (char const * filename, flag_t flags) 

{
	uint32_t version;
	signed fd;
	if ((fd = open (filename, O_BINARY|O_RDONLY)) == -1) 
	{
		error (1, errno, FILE_CANTOPEN, filename);
	}
	if (read (fd, &version, sizeof (version)) != sizeof (version)) 
	{
		error (1, errno, FILE_CANTREAD, filename);
	}
	close (fd);
	if (LE32TOH (version) == 0x60000000) 
	{
		return (nvmimage (filename, flags));
	}
	return (nvmchain (filename, flags));
}


/*====================================================================*
 *   
 *   int main (int argc, char const * argv []);
 *
 *   validate a PLC firmware image file; keep as little information 
 *   as possible in memory; this slows validation but minimizes the 
 *   resources needed at runtime; resource requirements do not grow
 *   with file size;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

int main (int argc, char const * argv []) 

{
	static char const * optv [] = 
	{
		"imqrsv",
		"file [file] [...]",
		"Qualcomm Atheros PLC Image File Validator",
		"i\tprint firmware identity string",
		"m\tdisplay manifest",
		"q\tsuppress messages",
		"r\tprint firmware revision string",
		"s\tprint SDRAM configuration blocks",
		"v\tverbose messages",
		(char const *) (0)
	};
	signed state = 0;
	flag_t flags = (flag_t)(0);
	signed c;
	optind = 1;
	while ((c = getoptv (argc, argv, optv)) != -1) 
	{
		switch ((char) (c)) 
		{
		case 'i':
			_setbits (flags, NVM_IDENTITY);
			break;
		case 'r':
			_setbits (flags, NVM_FIRMWARE);
			break;
		case 'm':
			_setbits (flags, NVM_MANIFEST);
			break;
		case 's':
			_setbits (flags, NVM_SDRAM);
			break;
		case 'q':
			_setbits (flags, NVM_SILENCE);
			break;
		case 'v':
			_setbits (flags, NVM_VERBOSE);
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	while ((argc) && (* argv)) 
	{
		if (chknvm (* argv, flags)) 
		{
			state = 1;
		}
		else if (_allclr (flags, (NVM_VERBOSE|NVM_SILENCE|NVM_MANIFEST|NVM_FIRMWARE|NVM_IDENTITY|NVM_SDRAM))) 
		{
			printf ("file %s looks good\n", * argv);
		}
		argc--;
		argv++;
	}
	return (state);
}

