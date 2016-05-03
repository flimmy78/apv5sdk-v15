/*====================================================================*
 *   
 *   Copyright (c) 2011 by Qualcomm Atheros.
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
#include <fcntl.h>
#include <errno.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../tools/getoptv.h"
#include "../tools/flags.h"
#include "../tools/error.h"
#include "../tools/files.h"
#include "../key/HPAVKey.h"
#include "../nvm/nvm.h"
#include "../pib/pib.h"

/*====================================================================*
 *   custom source files;
 *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/getoptv.c"
#include "../tools/putoptv.c"
#include "../tools/version.c"
#include "../tools/checksum32.c"
#include "../tools/fdchecksum32.c"
#include "../tools/checksum32.c"
#include "../tools/hexstring.c"
#include "../tools/hexdecode.c"
#include "../tools/strfbits.c"
#include "../tools/error.c"
#endif

#ifndef MAKEFILE
#include "../key/SHA256Reset.c"
#include "../key/SHA256Block.c"
#include "../key/SHA256Write.c"
#include "../key/SHA256Fetch.c"
#include "../key/HPAVKeyNID.c"
#include "../key/keys.c"
#endif

#ifndef MAKEFILE
#include "../pib/pibpeek1.c"
#include "../pib/pibpeek2.c"
#endif 

#ifndef MAKEFILE
#include "../nvm/manifest.c"
#endif 

/*====================================================================*
 *   program variables;
 *--------------------------------------------------------------------*/

static signed found = 0;

/*====================================================================*
 *
 *   signed pibimage1 (struct _file_ const * file, flag_t flags);
 *
 *   validate a disk-resident PIB image; read the header the verify
 *   the checksum and preferred Network Identifuier (NID); return 0
 *   on success or -1 on error; 
 *
 *   this is not a thorough check but it detects non-PIB images;
 *
 *   another function will be required to handle panther/lynx PIBs
 *   once their structure has been defined;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed pibimage1 (struct _file_ const * file, flag_t flags) 

{
	extern signed found;
	struct simple_pib simple_pib;
	uint8_t NID [HPAVKEY_NID_LEN];
	if (read (file->file, &simple_pib, sizeof (simple_pib)) != sizeof (simple_pib)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTREAD, file->name);
		}
		return (-1);
	}
	if (_anyset (flags, PIB_VERBOSE)) 
	{
		printf ("------- %s -------\n", file->name);
		if (pibpeek1 (&simple_pib)) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, PIB_BADVERSION, file->name);
			}
			return (-1);
		}
	}
	if (lseek (file->file, (off_t)(0)-sizeof (simple_pib), SEEK_CUR) == -1) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTHOME, file->name);
		}
		return (-1);
	}
	if (fdchecksum32 (file->file, LE16TOH (simple_pib.PIBLENGTH), 0)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, 0, PIB_BADCHECKSUM, file->name);
		}
		return (-1);
	}
	HPAVKeyNID (NID, simple_pib.NMK, simple_pib.PreferredNID [HPAVKEY_NID_LEN-1] >> 4);
	if (memcmp (NID, simple_pib.PreferredNID, sizeof (NID))) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, 0, PIB_BADNID, file->name);
		}
		return (-1);
	}
	found = 1;
	return (0);
}


/*====================================================================*
 *
 *   signed pibimage2 (struct _file_ const * file, uint32_t length, uint32_t checksum, flag_t flags);
 *
 *   validate a disk-resident PIB image; read the header the verify
 *   the checksum and preferred Network Identifuier (NID); return 0
 *   on success or -1 on error; 
 *
 *   this is not a thorough check but it detects non-PIB images;
 *
 *   another function will be required to handle panther/lynx PIBs
 *   once their structure has been defined;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed pibimage2 (struct _file_ const * file, uint32_t length, uint32_t checksum, flag_t flags) 

{
	extern signed found;
	struct simple_pib simple_pib;
	uint8_t NID [HPAVKEY_NID_LEN];
	if (read (file->file, &simple_pib, sizeof (simple_pib)) != sizeof (simple_pib)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTREAD, file->name);
		}
		return (-1);
	}
	if (_anyset (flags, PIB_VERBOSE)) 
	{
		printf ("------- %s -------\n", file->name);
		if (pibpeek2 (&simple_pib)) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, PIB_BADVERSION, file->name);
			}
			return (-1);
		}
	}
	if (lseek (file->file, (off_t)(0)-sizeof (simple_pib), SEEK_CUR) == -1) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTHOME, file->name);
		}
		return (-1);
	}
	if (fdchecksum32 (file->file, length, checksum)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, 0, PIB_BADCHECKSUM, file->name);
		}
		return (-1);
	}
	HPAVKeyNID (NID, simple_pib.NMK, simple_pib.PreferredNID [HPAVKEY_NID_LEN-1] >> 4);
	if (memcmp (NID, simple_pib.PreferredNID, sizeof (NID))) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, 0, PIB_BADNID, file->name);
		}
		return (-1);
	}
	found = 1;
	return (0);
}


/*====================================================================*
 *
 *   signed pibchain2 (struct _file_ const * file, flag_t flags);
 *
 *   open a panther/lynx parameter file and validate it;
 *
 *   traverse a panther/lynx image file looking for PIB images and
 *   validate each one; return 0 on success or -1 on error; errors
 *   occur due to an invalid image chain or a bad parameter block;
 *
 *   this implementation checks the parameter block without reading
 *   the entire block into memory; 
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed pibchain2 (struct _file_ const * file, flag_t flags) 

{
	struct nvm_header2 nvm_header;
	size_t origin = ~0;
	size_t offset = 0;
	signed module = 0;
	do 
	{
		if (read (file->file, &nvm_header, sizeof (nvm_header)) != sizeof (nvm_header)) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, errno, NVM_HDR_CANTREAD, file->name, module);
			}
			return (-1);
		}
		if (LE16TOH (nvm_header.MajorVersion) != 1) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, NVM_HDR_VERSION, file->name, module);
			}
			return (-1);
		}
		if (LE16TOH (nvm_header.MinorVersion) != 1) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, NVM_HDR_VERSION, file->name, module);
			}
			return (-1);
		}
		if (checksum32 (&nvm_header, sizeof (nvm_header), 0)) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, NVM_HDR_CHECKSUM, file->name, module);
			}
			return (-1);
		}
		if (LE32TOH (nvm_header.PrevHeader) != origin) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, NVM_HDR_LINK, file->name, module);
			}
			return (-1);
		}
		if (LE32TOH (nvm_header.ImageType) == NVM_IMAGE_PIB) 
		{

			return (pibimage2 (file, LE32TOH (nvm_header.ImageLength), nvm_header.ImageChecksum, flags));
		}
		if (LE32TOH (nvm_header.ImageType) == NVM_IMAGE_MANIFEST) 
		{
			if (_anyset (flags, PIB_MANIFEST)) 
			{
				off_t extent = LE32TOH (nvm_header.ImageLength);
				byte * memory = malloc (extent);
				if (!memory) 
				{
					error (1, 0, FILE_CANTLOAD, file->name);
				}
				if (read (file->file, memory, extent) != extent) 
				{
					error (1, errno, FILE_CANTREAD, file->name);
				}
				if (lseek (file->file, (off_t)(0)-extent, SEEK_CUR) == -1) 
				{
					error (1, errno, FILE_CANTHOME, file->name);
				}
				printf ("------- %s (%d) -------\n", file->name, module);
				if (manifest (memory, extent)) 
				{
					error (1, 0, "Bad manifest: %s", file->name);
				}
				free (memory);
				found = 1;
				break;
			}
		}
		if (fdchecksum32 (file->file, LE32TOH (nvm_header.ImageLength), nvm_header.ImageChecksum)) 
		{
			if (_allclr (flags, PIB_SILENCE)) 
			{
				error (0, 0, NVM_IMG_CHECKSUM, file->name, module);
			}
			return (-1);
		}
		origin = offset;
		offset = LE32TOH (nvm_header.NextHeader);
		module++;
	}
	while (~nvm_header.NextHeader);
	return (0);
}


/*====================================================================*
 *
 *   signed chkpib1 (char const * filename, flag_t flags);
 *
 *   open a named file and determine if it is a valid thunderbolt, 
 *   lightning, panther or lynx PIB; 
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *   
 *   Contributor(s):
 *	Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

static signed chkpib1 (char const * filename, flag_t flags) 

{
	struct _file_ file;
	uint32_t version;
	signed status = 0;
	file.name = filename;
	if ((file.file = open (file.name, O_BINARY|O_RDONLY)) == -1) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTOPEN, file.name);
		}
		return (-1);
	}
	if (read (file.file, &version, sizeof (version)) != sizeof (version)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTREAD, file.name);
		}
		return (-1);
	}
	if (lseek (file.file, 0, SEEK_SET)) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, errno, FILE_CANTHOME, file.name);
		}
		return (-1);
	}
	if (LE32TOH (version) == 0x60000000) 
	{
		if (_allclr (flags, PIB_SILENCE)) 
		{
			error (0, 0, FILE_WONTREAD, file.name);
		}
		return (-1);
	}
	if (LE32TOH (version) == 0x00010001) 
	{
		status = pibchain2 (&file, flags);
	}
	else 
	{
		status = pibimage1 (&file, flags);
	}
	close (file.file);
	return (status);
}


/*====================================================================*
 *   
 *   int main (int argc, char const * argv []);
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

int main (int argc, char const * argv []) 

{
	static char const * optv [] = 
	{
		"mqv",
		"file [file] [...]",
		"Qualcomm Atheros PLC Parameter File Inspector",
		"m\tdisplay manifest",
		"q\tquiet",
		"v\tverbose messages",
		(char const *) (0)
	};
	flag_t flags = (flag_t)(0);
	signed state = 0;
	signed c;
	optind = 1;
	while ((c = getoptv (argc, argv, optv)) != -1) 
	{
		switch (c) 
		{
		case 'm':
			_setbits (flags, PIB_MANIFEST);
			break;
		case 'q':
			_setbits (flags, PIB_SILENCE);
			break;
		case 'v':
			_setbits (flags, PIB_VERBOSE);
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	while ((argc) && (* argv)) 
	{
		found = errno = 0;
		if (chkpib1 (* argv, flags)) 
		{
			state = 1;
		}
		else if (!found) 
		{
			printf ("%s has no PIB\n", * argv);
		}
		else if (_allclr (flags, (PIB_VERBOSE | PIB_SILENCE | PIB_MANIFEST))) 
		{
			printf ("%s looks good\n", * argv);
		}
		argc--;
		argv++;
	}
	return (state);
}

