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

/*====================================================================*
 *
 *   signed FlashPTS (struct plc * plc);
 *
 *   plc.h
 *
 *   commit downloaded firmware and/or parameters to NVRAM using a
 *   VS_PTS_NVM message; flash-less devices will attempt to upload
 *   to their local host because they have no NVRAM; the host must
 *   be prepared to handle this situation; 
 *
 *   See the Atheros HomePlug AV Firmware Technical Reference Manual 
 *   for more information;
 *   
 *   This software and documentation is the property of Atheros 
 *   Corporation, Ocala, Florida. It is provided 'as is' without 
 *   expressed or implied warranty of any kind to anyone for any 
 *   reason. Atheros assumes no responsibility or liability for 
 *   errors or omissions in the software or documentation and 
 *   reserves the right to make changes without notification. 
 *   
 *   Atheros customers may modify and distribute the software 
 *   without obligation to Atheros. Since use of this software 
 *   is optional, users shall bear sole responsibility and 
 *   liability for any consequences of it's use. 
 *   
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *   Contributor(s):
 *      Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

#ifndef FLASHPTS_SOURCE
#define FLASHPTS_SOURCE

#include <stdint.h>
#include <memory.h>

#include "../tools/memory.h"
#include "../tools/error.h"
#include "../tools/flags.h"
#include "../plc/plc.h"

signed FlashPTS (struct plc * plc) 

{
	struct channel * channel = (struct channel *)(plc->channel);
	struct message * message = (struct message *)(plc->message);

#ifndef __GNUC__
#pragma pack (push,1)
#endif

	struct __packed vs_pts_nvm_request 
	{
		struct ethernet_std ethernet;
		struct qualcomm_std qualcomm;
		uint8_t MODULEID;
		uint8_t RESERVED;
		uint8_t DAK [HPAVKEY_DAK_LEN];
	}
	* request = (struct vs_pts_nvm_request *) (message);
	struct __packed vs_pts_nvm_confirm 
	{
		struct ethernet_std ethernet;
		struct qualcomm_std qualcomm;
		uint8_t MSTATUS;
		uint8_t MODULEID;
	}
	* confirm = (struct vs_pts_nvm_confirm *) (message);

#ifndef __GNUC__
#pragma pack (pop)
#endif

	Request (plc, "Flash device (PTS)");
	memset (message, 0, sizeof (* message));
	EthernetHeader (&request->ethernet, channel->peer, channel->host, HOMEPLUG_MTYPE);
	QualcommHeader (&request->qualcomm, 0, (VS_PTS_NVM | MMTYPE_REQ));
	request->MODULEID = plc->module;
	memcpy (request->DAK, plc->DAK, sizeof (request->DAK));
	plc->packetsize = (ETHER_MIN_LEN - ETHER_CRC_LEN);
	if (SendMME (plc) <= 0) 
	{
		error ((plc->flags & PLC_BAILOUT), ECANCELED, CHANNEL_CANTSEND);
		return (-1);
	}
	if (ReadMME (plc, 0, (VS_PTS_NVM | MMTYPE_CNF)) <= 0) 
	{
		error ((plc->flags & PLC_BAILOUT), ECANCELED, CHANNEL_CANTREAD);
		return (-1);
	}
	if (confirm->MSTATUS) 
	{
		Failure (plc, PLC_WONTDOIT);
		return (-1);
	}
	if (plc->module == 0x0C) 
	{

#if 1

/*
 *   this code is needed because the AR7400 behaves differently than the INT6x00 after flash
 *   memory is erased; the AR7400 returns to bootloader and sends HARs but ignores VS_SW_VER
 *   requests; consequently, if we are erasing flash memory and have not requested immediate
 *   return then we wait indefinitely for a VS_HST_ACTION.IND before proceding;
 */

		if (_allclr (plc->flags, PLC_QUICK_FLASH)) 
		{
			while (ReadMME (plc, 0, (VS_HST_ACTION | MMTYPE_IND)) <= 0);
			_setbits (plc->flags, PLC_QUICK_FLASH);
		}

#endif

	}
	return (0);
}


#endif

