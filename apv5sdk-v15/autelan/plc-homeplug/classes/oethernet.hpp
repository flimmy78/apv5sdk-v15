/*====================================================================*
 *
 *   oethernet.hpp - oethernet class definitions and declaration;
 *
 *   implement a standard Ethernet header consisting of peer address, 
 *   host address and ethertype; provide methods to encode and decode 
 *   external memory;
 *
 *   This software and documentation is the property of Intellon 
 *   Corporation, Ocala, Florida. It is provided 'as is' without 
 *   expressed or implied warranty of any kind to anyone for any 
 *   reason. Intellon assumes no responsibility or liability for 
 *   errors or omissions in the software or documentation and 
 *   reserves the right to make changes without notification. 
 *   
 *   Intellon customers may modify and distribute the software 
 *   without obligation to Intellon. Since use of this software 
 *   is optional, users shall bear sole responsibility and 
 *   liability for any consequences of it's use. 
 *
 *.  Intellon INT6000 Linux Toolkit for HomePlug AV;
 *:  Published 2006-2009 by Intellon Corp. ALL RIGHTS RESERVED;
 *;  For demonstration; Not for production use;
 *
 *   Contributor(s): 
 *	    Charles Maier <charles.maier@intellon.com>
 *
 *--------------------------------------------------------------------*/

#ifndef oETHERNET_HEADER
#define oETHERNET_HEADER

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <stdint.h>

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#if defined (__linux__)
#	include <net/ethernet.h>
#elif defined (__APPLE__)
#	include <net/ethernet.h>
#elif defined (__OpenBSD__)
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <net/if.h>
#	include <netinet/in.h>
#	include <netinet/if_ether.h>
#elif defined (WINPCAP)
#	include <net/ethernet.h>
#else
#error "Unknown environment"
#endif

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../classes/stdafx.hpp"

/*====================================================================*
 *   class constants;    
 *--------------------------------------------------------------------*/

#define oETHERNET_EMPTYCAST "00:00:00:00:00:00"
#define oETHERNET_BROADCAST "FF:FF:FF:FF:FF:FF"

/*====================================================================*
 *   class datatypes;
 *--------------------------------------------------------------------*/

typedef unsigned char byte;

/*====================================================================*
 *   class declaration;
 *--------------------------------------------------------------------*/

class __declspec (dllexport) oethernet 

{
public:
	oethernet (void);
	oethernet (uint16_t protocol);
	virtual ~ oethernet (void);
	size_t HeaderLength (void) const;
	const byte * HostAddress (void) const;
	const byte * PeerAddress (void) const;
	uint16_t Protocol (void) const;
	void * ExportHeader (void * memory) const;
	void * ExportPeerAddress (void * memory) const;
	void * ExportHostAddress (void * memory) const;
	void * ExportProtocol (void * memory) const;
	void const * ImportHeader (void const * memory);
	void const * ImportPeerAddress (void const * memory);
	void const * ImportHostAddress (void const * memory);
	void const * ImportProtocol (void const * memory);
	char const * HostAddressString (void) const;
	char const * PeerAddressString (void) const;
	char const * ProtocolString (void) const;
	oethernet & SetProtocol (uint16_t);
	oethernet & Print ();
	static byte const EmptycastAddress [ETHER_ADDR_LEN];
	static byte const BroadcastAddress [ETHER_ADDR_LEN];
private:
	byte mpeeraddr [ETHER_ADDR_LEN];
	byte mhostaddr [ETHER_ADDR_LEN];
	uint16_t mprotocol;
};

/*====================================================================*
 *   end declaration;
 *--------------------------------------------------------------------*/

#endif

