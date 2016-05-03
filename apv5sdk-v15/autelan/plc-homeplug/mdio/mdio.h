/*====================================================================*
 *
 *   mdio.h - mdio related definitions and declarations;
 *
 *.  Atheros Powerline Toolkit for HomePlug AV;
 *:  Published 2010 by Atheros Communications ALL RIGHTS RESERVED;
 *;  For demonstration and evaluation only; Not for production use;
 *
 *--------------------------------------------------------------------*/

#ifndef MDIO_HEADER
#define MDIO_HEADER

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

#define MDIO_VERBOSE (1 << 0)
#define MDIO_SILENCE (1 << 1)

#define MDIO16_USE(x) 		(((x) & 0x0001) << 0)
#define MDIO16_RSVD(x) 		(((x) & 0x001F) << 1)
#define MDIO16_CNT(x) 		(((x) & 0x03FF) << 6)
#define MDIO16_START(use,rsvd,cnt) 	(MDIO16_USE (use) | MDIO16_RSVD (rsvd) | MDIO16_CNT (cnt))

#define MDIO16_SRT(x)  		(((x) & 0x0003) << 0)
#define MDIO16_OP(x) 		(((x) & 0x0003) << 2)
#define MDIO16_PHY(x) 		(((x) & 0x001F) << 4)
#define MDIO16_REG(x)		(((x) & 0x001F) << 9)
#define MDIO16_TA(x)		(((x) & 0x0003) << 14)
#define MDIO16_INSTR(srt,op,phy,reg,ta) (MDIO16_SRT (srt) | MDIO16_OP (op) | MDIO16_PHY (phy) | MDIO16_REG (reg) | MDIO16_TA (ta))

#define MDIO32_HI_ADDR_SHIFT 9
#define MDIO32_LO_ADDR_SHIFT 1
#define MDIO32_HI_ADDR_MASK (0x000003FF << MDIO32_HI_ADDR_SHIFT)
#define MDIO32_LO_ADDR_MASK 0x000001FC

#define MDIO32_CODE_SHIFT 3
#define MDIO32_CODE_MASK (0x03 << CODE_SHIFT)
#define MDIO32_CODE_HI_ADDR 0x03   
#define MDIO32_CODE_LO_ADDR 0x02

#define MDIO32_HI_ADDR(a) ((a & MDIO32_HI_ADDR_MASK) >> MDIO32_HI_ADDR_SHIFT)
#define MDIO32_LO_ADDR(a) ((a & MDIO32_LO_ADDR_MASK) >> MDIO32_LO_ADDR_SHIFT)

#define MDIO32_INSTR(addr, data, mask) MDIO16_INSTR (1, 1, (MDIO32_CODE_HI_ADDR << MDIO32_CODE_SHIFT), 0x00, 2), MDIO32_HI_ADDR (addr), 0xFFFF, MDIO16_INSTR (1, 1, (MDIO32_CODE_LO_ADDR << MDIO32_CODE_SHIFT) | ((MDIO32_LO_ADDR (addr) & 0xE0) >> 5), MDIO32_LO_ADDR (addr) & 0x1F, 2), (data & 0x0000FFFF), mask & 0x0000FFFF, MDIO16_INSTR (1, 1, (MDIO32_CODE_LO_ADDR << MDIO32_CODE_SHIFT) | ((MDIO32_LO_ADDR (addr) & 0xE0) >> 5), (MDIO32_LO_ADDR (addr) & 0x1F) | 0x01, 2), (data & 0xFFFF0000) >> 16, (mask & 0xFFFF0000) >> 16

/*====================================================================*
 *
 *--------------------------------------------------------------------*/

#endif

