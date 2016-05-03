/*
*                Copyright 2002-2005, Marvell Semiconductor, Inc.
* This code contains confidential information of Marvell Semiconductor, Inc.
* No rights are granted herein under any patent, mask work right or copyright
* of Marvell or any third party.
* Marvell reserves the right at its sole discretion to request that this code
* be immediately returned to Marvell. This code is provided "as is".
* Marvell makes no warranties, express, implied or otherwise, regarding its
* accuracy, completeness or performance.
*/

/******************** (c) Marvell Semiconductor, Inc., 2001 *******************
*
* $HEADER$
*
* Purpose:
*    This file contains the implementation of the table that contains
*    information about external stations, including their MAC address,
*    power mode, and a state that indicates the current relationship
*    to the station using this table.
*
* Public Procedures:
*    extStaDb_AddSta      Add a station to the table
*    extStaDb_DelSta      Delete a station from the table
*    extStaDb_SetState    Set the state of an external station
*    extStaDb_GetState    Get the state of an external station
*
* Private Procedures:
*    LocateAddr           Looks for a given MAC address in the table
*    Hash                 Returns an index based on a hashing function call
*
* Notes:
*    None.
*
*****************************************************************************/

/*!
* \file    StaDb.c
* \brief   Stations Data Info Database module
*/
/*============================================================================= */
/*                               INCLUDE FILES */
/*============================================================================= */
//#include <stdlib.h>
#include "ap8xLnxIntf.h"
#include "wltypes.h"

#include "IEEE_types.h"
#include "mib.h"
#include "ds.h"
#include "osif.h"
#include "keyMgmtCommon.h"
#include "keyMgmt.h"
#include "tkip.h"
#include "StaDb.h"
#include "wldebug.h"

#include "qos.h"
#include "macmgmtap.h"
#include "macMgmtMlme.h"
//#include "macpriv.h"
#include "List.h"
#include "idList.h"
#include "mhsm.h"
#include "buildModes.h"
#include "ap8xLnxStaStats.h"
#include "wllog.h"

#ifdef USE_NEW_OSIF
SEM_HANDLE staSH;
SEM_HANDLE EthstaSH;
#else
#define staSH sysinfo_EXT_STA_SEM
#endif


/*============================================================================= */
/*                                DEFINITIONS */
/*============================================================================= */

/* */
/* Chosen as an optimal table size such that not too much memory is */
/* used up and hashing collisions are not overly frequent */
/* */


/*============================================================================= */
/*                              TYPE DEFINITIONS */
/*============================================================================= */

/* */
/* List elements that include information on an external station. */
/* */

/*============================================================================= */
/*                          MODULE LEVEL VARIABLES */
/*============================================================================= */

/* */
/* define the aging time */
/* */

/* */
/* Indicates if the database has been initialized */
/* */



/* */
/* An array of elements to store information on external stations */
/* */


/* */
/* The hash table; information on external stations is stored based on */
/* their MAC address. Since the MAC address is not a suitable index into */
/* an array of external stations, the address is input into a hashing */
/* function that returns a suitable index into the table. The size of the */
/* table has tradeoffs - the bigger it is, less collisions will occur */
/* from the hash function, but more memory will be used. The smaller it */
/* is, more collisions will occur, but less memory will be used. Each */
/* element in the hash table is a pointer to a list element that is used */
/* to store the information on the external station; if collisions occur */
/* for a given location in the hash table, then a list element is added */
/* to the element or elements already assigned to that location. */
/* */


/*------------------------------------------------------------------------*/
/* The following is a table used to determine if a requested state change */
/* of an external station is admissable, relative to the state currently  */
/* stored for that external station. The table is a square two            */
/* dimensional array where each row and column is associated with the     */
/* states given by the extStaDb_State_e enumeration type. Each column     */
/* represents a state currently stored for an external station; each row  */
/* represents a state requested to transition to. Hence, if the current   */
/* state is UNAUTHENTICATED, and a request made to transition to          */
/* ASSOCIATING state, the table indicates this is not admissable since    */
/* that entry in the table indicates NOT_AUTHENTICATED. Admissable        */
/* transitions are indicated by STATE_SUCCESS. It should be noted that    */
/* states dealing with association on apply to external stations that are */
/* also APs. The table does not check pertaining to APs vs. stations -    */
/* for example, it makes no sense to have stations associated with each   */
/* other. These checks must be made without the use of the table, which   */
/* is only used to verify state transitions.                              */
/*------------------------------------------------------------------------*/
/*============================================================================= */
/*                   PRIVATE PROCEDURES (ANSI Prototypes) */
/*============================================================================= */
static extStaDb_Status_e LocateAddr(vmacApInfo_t *vmacSta_p, IEEEtypes_MacAddr_t *Addr_p,
                                    ExtStaInfoItem_t **Item_p,
                                    UINT32 *Idx_p );
static UINT32 Hash ( UINT32 key );
static UINT32 Jenkins32BitMix(UINT32 key) ;
/*static UINT32 Wang32BitMix( UINT32 key ); */
/*============================================================================= */
/*                         CODED PUBLIC PROCEDURES */
/*============================================================================= */
extern void UpdateAssocStnData(UINT8 Aid, UINT8 ApMode);
extern void macMgmtMlme_DecrBonlyStnCnt(vmacApInfo_t *vmacSta_p, UINT8);
extern void FreePowerSaveQueue(UINT32 StnId);
extern void ProcessKeyMgmtData(vmacApInfo_t *vmacSta_p,void *,   IEEEtypes_MacAddr_t *,MhsmEvent_t *);
void extStaDb_ProcessAgeTick(UINT8 *data);
void ethStaDb_ProcessAgeTick(UINT8 *data);

int send_11n_aggregation_skb(struct net_device *netdev, extStaDb_StaInfo_t *pStaInfo, int force);
#ifdef WDS_FEATURE
void RemoveWdsPort(vmacApInfo_t *vmacSta_p,UINT8 *macAddr);
#endif
extern void CleanCounterClient(vmacApInfo_t *vmacSta_p );
extern void ClientStatistics(vmacApInfo_t *vmacSta_p, extStaDb_StaInfo_t *pStaInfo);
extern void HandleNProtectionMode(vmacApInfo_t *vmacSta_p );
#ifdef AMPDU_SUPPORT
extern void cleanupAmpduTx(vmacApInfo_t *vmacSta_p, UINT8 *macaddr);
#endif

#ifdef FLEX_TIME
UINT8 BGOnlyClientCnt=0;  /** use only in agflex mode **/
UINT8 AOnlyClientCnt=0;
#endif

extern void extStaDb_online_traffic_timer_fn(struct ieee80211vap *vap, extStaDb_StaInfo_t *StaInfo_p);

// zhouke add for ctl led
extern void  (*apv7_gpio_set_value)(unsigned,int);

#define AUTELAN_SET_BULE_LED_ON  apv7_gpio_set_value(29,0);
#define AUTELAN_SET_BULE_LED_OFF  apv7_gpio_set_value(29,1);
#define AUTELAN_SET_GREEN_LED_ON  apv7_gpio_set_value(30,1);
#define AUTELAN_SET_GREEN_LED_OFF  apv7_gpio_set_value(30,0);
#define AUTELAN_SET_RED_LED_ON  apv7_gpio_set_value(60,0);
#define AUTELAN_SET_RED_LED_OFF  apv7_gpio_set_value(60,1);

#define AUTELAN_SET_BULE_LED_BLINK_ON apv7_gpio_set_blink(29,1);
#define AUTELAN_SET_BULE_LED_BLINK_OFF apv7_gpio_set_blink(29,0);
#define AUTELAN_SET_GREEN_LED_BLINK_ON apv7_gpio_set_blink(30,1);
#define AUTELAN_SET_GREEN_LED_BLINK_OFF apv7_gpio_set_blink(30,0);
#define AUTELAN_SET_RED_LED_BLINK_ON apv7_gpio_set_blink(60,1);
#define AUTELAN_SET_RED_LED_BLINK_OFF apv7_gpio_set_blink(60,0);

//end

/******************************************************************************
*
* Name: extStaDb_Init
*
* Description:
*    Routine to initial the structures used in the external stations table.
*
* Conditions For Use:
*    None.
*
* Arguments:
*    None.
*
* Return Value:
*    Status indicating success or failure.
*
* Notes:
*    None.
*
* PDL:
*    Initialize the array of pointers to station elements to NULL
*    Set up the list of free structures which will be used to store
*       information on external stations
*    Initialized the semaphore used to govern access to the table
*    Set the initialized flag to true
*    Return success status
* END PDL
*
*****************************************************************************/
/*!
* Station database initialization 
*/
extern WL_STATUS extStaDb_Init(vmacApInfo_t *vmacSta_p,UINT16 MaxStns )
{
    UINT32 i;

    if (NULL == vmacSta_p)
        return OS_FAIL;
    
    if(vmacSta_p->master)
        return(OS_SUCCESS);
    if(vmacSta_p->StaCtl == NULL)
    {
        vmacSta_p->StaCtl= (struct STADB_CTL *)malloc(sizeof(struct STADB_CTL));
        if(vmacSta_p->StaCtl == NULL)
        {
            printk("fail to alloc memory\n");
            return OS_FAIL;
        }
        memset(vmacSta_p->StaCtl, 0, sizeof(struct STADB_CTL));
    }

    vmacSta_p->StaCtl->MaxStaSupported = MaxStns;
    vmacSta_p->StaCtl->aging_time_in_minutes = 40*10 / AGING_TIMER_VALUE_IN_SECONDS;    //zhouke add,3*60 change 400 s;

    /*pre-allocate sta datebase */
    if ( vmacSta_p->StaCtl->ExtStaInfoDb == NULL )
    {
        vmacSta_p->StaCtl->ExtStaInfoDb = (ExtStaInfoItem_t *)malloc(MaxStns *sizeof(ExtStaInfoItem_t));
        if (vmacSta_p->StaCtl->ExtStaInfoDb == NULL)
        {
            printk("fail to alloc memory\n");
            return OS_FAIL;
        }
        memset(vmacSta_p->StaCtl->ExtStaInfoDb, 0, MaxStns *sizeof(ExtStaInfoItem_t));
    }

    /*----------------------------------------------------------------*/
    /* Initial the array of pointers to station information elements; */
    /* initially, the pointers all point to nothing.                  */
    /*----------------------------------------------------------------*/
    for (i = 0; i < EXT_STA_TABLE_SIZE; i++)
    {
        vmacSta_p->StaCtl->ExtStaInfoDb_p[i] = NULL;
    }

    ListInit(&vmacSta_p->StaCtl->FreeStaList);
    ListInit(&vmacSta_p->StaCtl->StaList);
    /*-------------------------------------------------------------*/
    /* Set up the list of initially free elements that are used to */
    /* record external station information.                        */
    /*-------------------------------------------------------------*/
    for ( i = 0; i < MaxStns; i++ )
    {
        vmacSta_p->StaCtl->ExtStaInfoDb[i].nxt = NULL;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].prv = NULL;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].nxt_ht = NULL;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].prv_ht = NULL;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.AP = FALSE;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.State = UNAUTHENTICATED;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.PwrMode = PWR_MODE_ACTIVE;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.TimeStamp = 30;
        vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.ClientMode = 0;
        memset(&vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.Addr, 0, sizeof(IEEEtypes_MacAddr_t));
        TimerInit(&vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.keyMgmtHskHsm.keyTimer);
        TimerInit(&vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.mgtAssoc.timer);
        TimerInit(&vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.mgtAuthReq.timer);
        TimerInit(&vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.mgtAuthRsp.timer);
        ListPutItem(&vmacSta_p->StaCtl->FreeStaList, (ListItem*)(vmacSta_p->StaCtl->ExtStaInfoDb+i));
    }

    /*----------------------------------------------------------------------*/
    /* Initialize the semaphore and initialize flag and then return status. */
    /*----------------------------------------------------------------------*/
#ifdef USE_NEW_OSIF
    os_SemaphoreInit(&staSH, 1);
#else
    os_SemaphoreInit(sysinfo_EXT_STA_SEM, 1);
    os_SemaphoreInit(sysinfo_STALIST_SEM, 1);
#endif

    vmacSta_p->StaCtl->Initialized = TRUE;
    return(OS_SUCCESS);
}

/*!
* Free Station database memory. 
* @param ExtStaInfoDb Pointer to a station database
* @return void         
*/

extern void extStaDb_Cleanup(vmacApInfo_t *vmacSta_p)
{
    if (NULL == vmacSta_p)
        return;
    if(vmacSta_p->master)
        return;
    if (vmacSta_p->StaCtl->ExtStaInfoDb)
        free(vmacSta_p->StaCtl->ExtStaInfoDb);
    if(vmacSta_p->StaCtl)
        free(vmacSta_p->StaCtl);
    return;
}


/******************************************************************************
*
* Name: extStaDb_AddSta
*
* Description:
*    This routine adds a station or AP to the external station table.
*
* Conditions For Use:
*    External station table has been initialized.
*
* Arguments:
*    Arg1 (i  ): StaInfo - Pointer to a structure containing information
*                          about the station being added
*
* Return Value:
*    Status indicating the results of the operation; possbile values are:
*
*       NOT_INITIALIZED
*       STATION_EXISTS_ERROR
*       TABLE_FULL_ERROR
*       ADD_SUCCESS
*
* Notes:
*    None.
*
* PDL:
*    If the database has not been initialized Then
*       Return an uninitialized status
*    End If
*
*    Get the semaphore for access to the database
*    Call LocateAddr to get a pointer to the place in the table to add
*       the new station
*    If a station with the given MAC address is already in the table Then
*       Return the semaphore
*       Return a status indicating the station already exists
*    End If
*
*    If the list of available structures is not empty Then
*       Remove an available structure from the free list, fill it out with
*          the information for the new station and add it to the location
*          based on the pointer given by the LocateAddr call
*       Return the semaphore
*       Return success status
*    Else
*       Return a storage error
*    End If
* END PDL
*
*****************************************************************************/

/*!
* Add a station to the station database 
* @param StaInfo_p Pointer to a station info structure
* @return NOT_INITIALIZED,      
*         STATION_EXISTS_ERROR, 
*         TABLE_FULL_ERROR,     
*         ADD_SUCCESS          
*/
extern extStaDb_Status_e extStaDb_AddSta (vmacApInfo_t *vmac_p, extStaDb_StaInfo_t *StaInfo_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    ListItem *tmp;
    vmacApInfo_t *vmacSta_p;
    struct wlprivate *priv = NULL;

    if (NULL == vmac_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    priv = NETDEV_PRIV_P(struct wlprivate, vmacSta_p->dev);

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find a spot where the station that is to be added  */
    /* can be placed; if the station is already in the table, give back */
    /* the semaphore and return status.                                 */
    /*------------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,&(StaInfo_p->Addr), &item_p, &idx)) !=
        LOCATE_FAILURE)
    {
        os_SemaphorePut(staSH);
        return(STATION_EXISTS_ERROR);
    }

    /*---------------------------------------------------------------*/
    /* Get a structure off of the free list, fill it out with the    */
    /* information about the new station, and put it in the location */
    /* found above.                                                  */
    /*---------------------------------------------------------------*/

    tmp = ListGetItem(&vmacSta_p->StaCtl->FreeStaList);
    if(tmp)
    {
        ExtStaInfoItem_t *search_p = vmacSta_p->StaCtl->ExtStaInfoDb_p[idx];
        item_p = (ExtStaInfoItem_t *)tmp;
        memset(&item_p->StaInfo, 0, sizeof(extStaDb_StaInfo_t));
        memcpy(&item_p->StaInfo.Addr, &StaInfo_p->Addr,
            sizeof(IEEEtypes_MacAddr_t));
        memcpy(&item_p->StaInfo.Bssid, &StaInfo_p->Bssid,
            sizeof(IEEEtypes_MacAddr_t));
        item_p->StaInfo.State   = StaInfo_p->State;
        item_p->StaInfo.PwrMode = StaInfo_p->PwrMode;
        item_p->StaInfo.StnId   = StaInfo_p->StnId;
        item_p->StaInfo.Aid     = StaInfo_p->Aid;
        item_p->StaInfo.ApMode = StaInfo_p->ApMode;
        item_p->StaInfo.ClientMode = StaInfo_p->ClientMode;
#ifdef WDS_FEATURE
        item_p->StaInfo.AP      = StaInfo_p->AP;
        item_p->StaInfo.wdsInfo = StaInfo_p->wdsInfo;
        item_p->StaInfo.wdsPortInfo = StaInfo_p->wdsPortInfo;
#else
#ifdef MBSS
        item_p->StaInfo.AP      = StaInfo_p->AP;
#endif
#endif
#ifdef CLIENT_SUPPORT
        item_p->StaInfo.Client= StaInfo_p->Client;
#endif
        item_p->StaInfo.IsStaMSTA = 0;
        item_p->StaInfo.StaType = 0;
        memset(&item_p->StaInfo.aggr11n , 0, sizeof(Aggr11n));
        memset(&item_p->StaInfo.HtElem , 0, sizeof(IEEEtypes_HT_Element_t));
        memset(&item_p->StaInfo.AddHtElme , 0, sizeof(IEEEtypes_Add_HT_Element_t));
        spin_lock_init(&item_p->StaInfo.aggr11n.Lock);
        skb_queue_head_init(&item_p->StaInfo.aggr11n.txQ);
        memset(&item_p->StaInfo.DeFragBufInfo, 0, sizeof(DeFragBufInfo_t));
        item_p->StaInfo.mib_p= StaInfo_p->mib_p;
        item_p->StaInfo.dev = StaInfo_p->dev;
        if(search_p)
        {   /*if hash table index idx already exist */
            while(search_p->nxt_ht)
            {
                search_p = search_p->nxt_ht;
            }
            search_p->nxt_ht = item_p;
            item_p->prv_ht = search_p;
            item_p->nxt_ht = NULL;
        }
        else
        {   /*put item to hash table idx */
            item_p->nxt_ht = item_p->prv_ht = NULL;
            vmacSta_p->StaCtl->ExtStaInfoDb_p[idx] = item_p;
        }

        if(vmacSta_p->StaCtl->StaList.cnt == 0)     // zhouke add for ctl led
        {
            AUTELAN_SET_BULE_LED_ON
            AUTELAN_SET_GREEN_LED_OFF
        }

        ListPutItem(&vmacSta_p->StaCtl->StaList,tmp);
        item_p->StaInfo.TimeStamp = (vmacSta_p->auth_time_in_minutes/AGING_TIMER_VALUE_IN_SECONDS);

        /*-------------------------------------------------------*/
        /* Finished - give back the semaphore and return status. */
        /*-------------------------------------------------------*/
        os_SemaphorePut(staSH);
        return (ADD_SUCCESS);
    }
    else
    {
        /*-------------------------------------------------------------*/
        /* There is no room in the table to add the station; give back */
        /* the semaphore and return status.                            */
        /*-------------------------------------------------------------*/
        /* Remove the least active station, and use the space for the new
        * one */
        os_SemaphorePut(staSH);
        return (TABLE_FULL_ERROR);
    }
}

/*!
* station aging timer tick process, will age out out station when age timer expired 
* @param data Pointer to user defined data
*/

void extStaDb_ProcessKeepAliveTimer(UINT8 *data)
{
    vmacApInfo_t *vmacSta_p = NULL;
    if (NULL == data)
        return;
    vmacSta_p = (vmacApInfo_t *)data;
    if(!vmacSta_p->download)
    {
        //wlFwSetKeepAliveTick(vmacSta_p->dev,0);
    }

    TimerRearm(&vmacSta_p->KeepAliveTimer, AGING_TIMER_VALUE_IN_SECONDS);
    return;
}

void Disable_extStaDb_ProcessKeepAliveTimer(vmacApInfo_t *vmacSta_p)
{
    if (NULL == vmacSta_p)
    {
        WlLogPrint(MARVEL_DEBUG_PANIC, __func__,"netdev is NULL");
        return;
    }
    TimerRemove(&vmacSta_p->KeepAliveTimer);
    return;
}

extern void extStaDb_ProcessKeepAliveTimerInit(vmacApInfo_t *vmacSta_p)
{
    if (NULL == vmacSta_p)
        return;
    TimerInit(&vmacSta_p->KeepAliveTimer);
    TimerFireIn(&vmacSta_p->KeepAliveTimer, 1, &extStaDb_ProcessKeepAliveTimer, (unsigned char *)vmacSta_p, AGING_TIMER_VALUE_IN_SECONDS );
    return;
}

void stationAging(UINT8 *data)
{
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == data)
        return;
    vmacSta_p = (vmacApInfo_t *)data;
    extStaDb_ProcessAgeTick(data);
    ethStaDb_ProcessAgeTick(data);
    TimerRearm(&vmacSta_p->AgingTimer, AGING_TIMER_VALUE_IN_SECONDS * 10);
    return;
}

void Disable_stationAgingTimer(vmacApInfo_t *vmacSta_p)
{
    if (NULL == vmacSta_p)
    {
        WlLogPrint(MARVEL_DEBUG_PANIC, __func__,"netdev is NULL");
        return;
    }
    TimerRemove(&vmacSta_p->AgingTimer);
    return;
}

extern void extStaDb_AgingTimerInit(vmacApInfo_t *vmacSta_p)
{
    /** Start aging timer */
    if (NULL == vmacSta_p)
        return;
    TimerInit(&vmacSta_p->AgingTimer);
    TimerFireIn(&vmacSta_p->AgingTimer, 1, &stationAging, (unsigned char *)vmacSta_p, AGING_TIMER_VALUE_IN_SECONDS * 10 );
    return;
}

void extStaDb_ProcessAgeTick(UINT8 *data)
{
    vmacApInfo_t *vmacSta_p = NULL;
    ExtStaInfoItem_t *Curr_p, *Item_p;
    extStaDb_StaInfo_t *StaInfo_p;
    UINT32 count=0;

    /* Traverse the aging list,
    *  Remove the aged ones, and send proper Disassociate message or
    * Deauthenticate message
    */
    WLDBG_INFO(DBG_LEVEL_10, "extStaDb_ProcessAgeTick \n");

    vmacSta_p = (vmacApInfo_t *)data;
    if (NULL == vmacSta_p)
    {
        return;
    }

    Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
    while (Curr_p != NULL)
    {
        if(count++>EXT_STA_TABLE_SIZE)
            break;
        Item_p = Curr_p;
#ifdef SC_PALLADIUM  /* Disable ageing for Palladium */
        if (1) //--Item_p->StaInfo.TimeStamp)
#else
        if ((Item_p->StaInfo.StaType & 0x02) == 0x02)
        {
            Curr_p = Curr_p->nxt;
            continue;
        }
        if (--Item_p->StaInfo.TimeStamp)
#endif
        {
            Curr_p = Curr_p->nxt;
        }
        else
        {
            /* Item can be aged, This involves removing the item
            * from the list and also, send message to the station
            */
            if ( (StaInfo_p = extStaDb_GetStaInfo(vmacSta_p,&(Item_p->StaInfo.Addr), 2/*STADB_NO_BLOCK*/)) == NULL )
            {
                /* Station not known, do nothing */
                return ;
            }
            WlLogPrint(MARVEL_DEBUG_WARNING,__func__,"AGEING TIME OUT FROM %02X:%02X:%02X:%02X:%02X:%02X\n",\
                    StaInfo_p->Addr[0],StaInfo_p->Addr[1],StaInfo_p->Addr[2],StaInfo_p->Addr[3],\
                    StaInfo_p->Addr[4],StaInfo_p->Addr[5]);
#ifdef CLIENT_SUPPORT
            if ( StaInfo_p->AP   || StaInfo_p->Client)
#else
            if ( StaInfo_p->AP )
#endif
            {
                if(0)//AgeOutAP)  /** No aging for WDS for now **/
                {
#ifdef WDS_FEATURE
                    RemoveWdsPort(vmacSta_p,(UINT8 *)&(StaInfo_p->Addr));
#endif
                    FreeAid(StaInfo_p->Aid);
                    ResetAid(StaInfo_p->StnId, StaInfo_p->Aid);
                    StaInfo_p->Aid = 0;
                    extStaDb_DelSta(vmacSta_p,&(Item_p->StaInfo.Addr),0);
                    FreeStnId(StaInfo_p->StnId);
                    return;
                }
                else
                {
                    Curr_p = Curr_p->nxt;
                    continue;
                }
            }
            else
            {
#ifdef AP_URPTR
                /* Clean up UR hash table for this STA */
                urHash_RemoveSta(vmacSta_p,&(Item_p->StaInfo.Addr), 0);
#endif
                if ( StaInfo_p->State == ASSOCIATED )
                {
#ifdef AMPDU_SUPPORT
                    cleanupAmpduTx(vmacSta_p,(UINT8 *)&Item_p->StaInfo.Addr);
#endif
                    macMgmtMlme_SendDeauthenticateMsg(vmacSta_p,&Item_p->StaInfo.Addr, Item_p->StaInfo.StnId,
                        IEEEtypes_REASON_DISASSOC_INACTIVE );
                    FreeAid(StaInfo_p->Aid);
                    ResetAid(StaInfo_p->StnId, StaInfo_p->Aid);
                    StaInfo_p->Aid = 0;

                }
                if (StaInfo_p->PwrMode == PWR_MODE_PWR_SAVE)
                {
                    if (vmacSta_p->PwrSaveStnCnt)
                        vmacSta_p->PwrSaveStnCnt--;
                    StaInfo_p->PwrMode= PWR_MODE_ACTIVE;
                }
                if (StaInfo_p->ClientMode == BONLY_MODE)
                    macMgmtMlme_DecrBonlyStnCnt(vmacSta_p,0);


            }
            if (StaInfo_p->Aid !=0)
            {
                FreeAid(StaInfo_p->Aid);
                ResetAid(StaInfo_p->StnId, StaInfo_p->Aid);
                StaInfo_p->Aid = 0;
            }
            StaInfo_p->State = UNAUTHENTICATED;
#ifdef APEVT_STA_ASSOC_SUPPORT

            apEvtRun(APEVT_STA_ASSOC, (uint8 *)&StaInfo.Addr, APEVT_STA_DEAUTHENTICATED);
#endif
            extStaDb_DelSta(vmacSta_p,&(Item_p->StaInfo.Addr),0);
            FreeStnId(StaInfo_p->StnId);

            /* need not update the current as, this is already done
            * as a result of previous two item_p updates */
            WLDBG_EXIT(DBG_LEVEL_10);

            return;
        }
        /*Curr_p = Curr_p->nxt; */
    }
}

void extStaDb_RemoveAllStns(vmacApInfo_t *vmac_p,UINT16 Reason)
{
    ExtStaInfoItem_t *Curr_p, *Item_p;
    extStaDb_StaInfo_t *StaInfo_p;
    vmacApInfo_t *vmacSta_p;
    int removeall=0;

    if (NULL == vmac_p)
        return;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
    {
        vmacSta_p = vmac_p;
        //this is not from vitual interface, need to remove all.
        removeall = 1;
    }
    /* Traverse the aging list,
    * Remove the aged ones, and send proper Disassociate message or
    * Deauthenticate message
    */
    Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
    while ( Curr_p != NULL )
    {
        Item_p = Curr_p;
        {
            /* Item can be aged, This involves removing the item
            * from the list and also, send message to the station
            * Only remove station in the BSS
            */
            if ( (StaInfo_p = extStaDb_GetStaInfo(vmac_p,&(Item_p->StaInfo.Addr), 0)) == NULL ||memcmp(&vmac_p->macBssId, &StaInfo_p->Bssid,
                sizeof(IEEEtypes_MacAddr_t)) )
            {
                //if call from wdev, remove it too.
                if(removeall)
                {               
                    extStaDb_DelSta(vmac_p,&(Item_p->StaInfo.Addr), 0);
                    Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
                    continue;
                }
                /* Station not known, do nothing */
                Curr_p = Curr_p->nxt;
                break;
            }
            if ( StaInfo_p->State == ASSOCIATED )
            {
                macMgmtMlme_SendDeauthenticateMsg(vmac_p,&Item_p->StaInfo.Addr, Item_p->StaInfo.StnId,
                    Reason );
                /* remove the Mac address from the ethernet MAC address table */
                macMgmtCleanUp(vmac_p,StaInfo_p); 


            }
            StaInfo_p->State = UNAUTHENTICATED;
            FreeStnId(StaInfo_p->StnId);
            extStaDb_DelSta(vmac_p,&(Item_p->StaInfo.Addr), 0);
            Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
        }
    }
}
/******************************************************************************
*
* Name: extStaDb_DelSta
*
* Description:
*    This routine deletes a station from the external station table.
*
* Conditions For Use:
*    External station table has been initialized.
*
* Arguments:
*    Arg1 (i  ): Addr_p  - Pointer to the MAC address of the station to be
*                          deleted
*
* Return Value:
*    Status indicating the results of the operation; possbile values are:
*
*       NOT_INITIALIZED
*       LOCATE_FAILURE
*       DEL_SUCCESS
*
* Notes:
*    None.
*
* PDL:
*    If the database has not been initialized Then
*       Return an uninitialized status
*    End If
*
*    Get the semaphore for access to the database
*    Call LocateAddr to get a pointer to the structure in the table
*       corresponding to the given station that is to be removed
*    If a station could not be located in the table Then
*       Return the semaphore
*       Return status indicating failure to locate the station
*    End If
*
*    Remove that found structure from the table and put it back on the
*       free list
*    Return the semaphore
*    Return success status
* END PDL
*
*****************************************************************************/
/*!
* Remove a station from the station database 
* @param Addr_p Pointer to a station mac address
* @return NOT_INITIALIZED,      
*         LOCATE_FAILURE, 
*         DEL_SUCCESS     
*/
extern extStaDb_Status_e extStaDb_DelSta (vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p, int option )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;
	MIB_802DOT11 *mib;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
	mib = vmacSta_p->Mib802dot11;		
#ifdef AP_URPTR
    urHash_RemoveSta(vmacSta_p,Addr_p, 1);
#endif
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    if ( option != STADB_NO_BLOCK )
        os_SemaphoreGet(staSH);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be deleted; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        if ( option != STADB_NO_BLOCK )
            os_SemaphorePut(staSH);
        return(result);
    }
	
	/*Everytime delete client, remove entry in multicast proxy list too*/
	if (*(mib->mib_MCastPrxy))
		McastProxyUCastAddrRemove(vmacSta_p, Addr_p);
    /*----------------------------------------*/
    /* Put the element back on the free list. */
    /*----------------------------------------*/
    {
        ExtStaInfoItem_t *search_p = vmacSta_p->StaCtl->ExtStaInfoDb_p[idx];
        if(search_p)
        {

            while (memcmp(&(search_p->StaInfo.Addr),
                item_p->StaInfo.Addr, 6*sizeof(IEEEtypes_Addr_t)))
            {

                search_p = search_p->nxt_ht;
            }

        }
        if(search_p && item_p)
        {
            if (search_p->prv_ht && search_p->nxt_ht)
            {   /*middle element */

                item_p->nxt_ht->prv_ht = item_p->prv_ht;
                item_p->prv_ht->nxt_ht = item_p->nxt_ht;
            }
            else
            {
                if(search_p->prv_ht)
                {   /*this is tail */
                    search_p->prv_ht->nxt_ht = NULL;
                }
                else if(search_p->nxt_ht)
                {   /*this is header */
                    search_p->nxt_ht->prv_ht = NULL;
                    vmacSta_p->StaCtl->ExtStaInfoDb_p[idx] =search_p->nxt_ht;
                }
                else
                {   /*one and only */
                    vmacSta_p->StaCtl->ExtStaInfoDb_p[idx] = NULL;
                }
            }

            item_p->nxt_ht = item_p->prv_ht = NULL;
        }

    }
    TimerRemove(&item_p->StaInfo.mgtAssoc.timer);
    TimerRemove(&item_p->StaInfo.mgtAuthReq.timer);
    TimerRemove(&item_p->StaInfo.mgtAuthRsp.timer);
    TimerRemove(&item_p->StaInfo.keyMgmtHskHsm.timer);
    TimerRemove(&item_p->StaInfo.keyMgmtHskHsm.keyTimer);
    /*add by zhanxuechao for online traffic process*/
    TimerRemove(&item_p->StaInfo.online_traffic_timer);
    skb_queue_purge(&item_p->StaInfo.aggr11n.txQ);

    ethStaDb_RemoveStaPerWlan(vmacSta_p, Addr_p);
    ListPutItem(&vmacSta_p->StaCtl->FreeStaList, ListRmvItem(&vmacSta_p->StaCtl->StaList,(ListItem *)item_p));

    if(vmacSta_p->StaCtl->StaList.cnt == 0)     // zhouke add for ctl led
    {
        AUTELAN_SET_BULE_LED_OFF
        AUTELAN_SET_GREEN_LED_ON
    }

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    if ( option != STADB_NO_BLOCK )
        os_SemaphorePut(staSH);
#ifdef NPROTECTION
    extStaDb_entries(vmac_p, 0);
    HandleNProtectionMode(vmac_p);
#endif
    return(DEL_SUCCESS);
}
/******************************************************************************
*
* Name: extStaDb_GetStnInfo
*
* Description:
*    This routine attempts to retrieve the state for the given MAC address.
*
* Conditions For Use:
*    External station table has been initialized.
*
* Arguments:
*    Arg1 (i  ): Addr_p  - Pointer to the MAC address of the station for
*                          which the state is to be retrieved
*    Arg2 (  o): State_p - Pointer to the variable that will contain the
*                          requested station information
*
* Return Value:
*    Status indicating the results of the operation; possbile values are:
*
*       NOT_INITIALIZED
*       LOCATE_FAILURE
*       STATE_SUCCESS
*
* Notes:
*    None.
*
* PDL:
*    If the database has not been initialized Then
*       Return an uninitialized status
*    End If
*
*    Get the semaphore for access to the database
*    Call LocateAddr to get a pointer to the structure in the table
*       corresponding to the given station
*    If a station could not be located in the table Then
*       Return the semaphore
*       Return status indicating failure to locate the station
*    End If
*
*    Retrieve the station info for the given station
*
*    Return the semaphore
*    Return status indicating results of the operation
* END PDL
*
*****************************************************************************/
/*!
* Check to see if station exist in the station database 
* @param Addr_p Pointer to a station mac address
* @param resetAgeTime Flag to indicate reset the aging time
* @return Pointer to the station info struct,      
*         NULL 
*/
extern extStaDb_StaInfo_t  *extStaDb_GetStaInfo(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p, int option)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    extStaDb_StaInfo_t *StaInfo_p =NULL; 
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return NULL;

    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return NULL;
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    if ( option != STADB_NO_BLOCK )
        os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /* if not found, give back the semaphore and return error status.   */
    /*------------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        if ( option != STADB_NO_BLOCK )
            os_SemaphorePut(staSH);
        return NULL;
    }

    /*-------------------------------------------*/
    /* Fill out the requested state information. */
    /*-------------------------------------------*/
    StaInfo_p = &item_p->StaInfo;
    
#ifdef WDS_FEATURE
    if ( option && (StaInfo_p->State == ASSOCIATED || StaInfo_p->AP))
#else
    if ( option && StaInfo_p->State == ASSOCIATED )
#endif
    {
        /***********zhouke add ,auth time *************/
        if(ieee80211_node_is_authorize(StaInfo_p))
        {
            item_p->StaInfo.TimeStamp = vmacSta_p->StaCtl->aging_time_in_minutes;  /* AGING_TIME; */
        }
        /***************** end *********************/ 
    }

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    if ( option != STADB_NO_BLOCK )
        os_SemaphorePut(staSH);
    
    if(StaInfo_p->Client)
        return StaInfo_p;
    else
    {
        //for AP filter out station not from same BSSID
        //option 2 is to check other VAP's station
        if(option == 2)
        {
            return StaInfo_p;
        }
        if(!memcmp(&vmac_p->macBssId, &StaInfo_p->Bssid,sizeof(IEEEtypes_MacAddr_t)))
            return StaInfo_p;
        else
        {
            return NULL;
        }
    }
}


/*============================================================================= */
/*                         CODED PRIVATE PROCEDURES */
/*============================================================================= */

/******************************************************************************
*
* Name: Jenkins32BitMix
*
* Description:
*   Routine that performs a hash on a 32 bit number.
*
* Conditions For Use:
*   None.
*
* Arguments:
*    Arg1 (i  ): Key - 32 bit value that will be hashed
*
* Return Value:
*   The index produced from hashing
*
* Notes:
*   This is just experimental at this time.
*
* PDL:
*    Implement the Jenkins 32 bit mix hash function
* END PDL
*
*****************************************************************************/
inline static UINT32 Jenkins32BitMix(UINT32 Key)
{
    Key += (Key << 12);
    Key ^= (Key >> 22);
    Key += (Key << 4);
    Key ^= (Key >> 9);
    Key += (Key << 10);
    Key ^= (Key >> 2);
    Key += (Key << 7);
    Key ^= (Key >> 12);

    return(Key);
}

/******************************************************************************
*
* Name: Wang32BitMix
*
* Description:
*   Routine that performs a hash on a 32 bit number.
*
* Conditions For Use:
*   None.
*
* Arguments:
*    Arg1 (i  ): Key - 32 bit value that will be hashed
*
* Return Value:
*   The index produced from hashing
*
* Notes:
*   This is just experimental at this time.
*
* PDL:
*    Implement the Wang 32 bit mix hash function
* END PDL
*
*****************************************************************************/
UINT32 Wang32BitMix( UINT32 Key )
{
    Key += ~(Key << 15);
    Key ^= (Key >> 10);
    Key += (Key << 3);
    Key ^= (Key >> 6);
    Key += ~(Key << 11);
    Key ^= (Key >> 16);

    return(Key);
}
UINT32 (*Wang32BitMixFp)( UINT32 Key ) = Wang32BitMix;

/******************************************************************************
*
* Name: Hash
*
* Description:
*   Routine that takes a key, performs a hash on that key, and then
*   scales the resulting index down to the size of the hash table. In this
*   case, the hash table is the table of external stations.
*
* Conditions For Use:
*   None.
*
* Arguments:
*    Arg1 (i  ): Key - 32 bit value that will be hashed
*
* Return Value:
*   The index produced from hashing and scaling
*
* Notes:
*   The values returned by the hashing functions are 32 bit unsigned
*   values. Hence, to get an index for the hash table, the result from
*   the hash must be normalized by the largest 32 bit number and then
*   multiplied by the table size to yield an index appropriate for the
*   hash table.
*
* PDL:
*    Call a routine to do the hash
*    Scale and return the result
* END PDL
*
*****************************************************************************/
inline static UINT32 Hash ( UINT32 Key )
{
    unsigned int result;

    /*-------------------------------------------------------------*/
    /* Call a hash function; the current routines are experimental */
    /* and not final.                                              */
    /*-------------------------------------------------------------*/
    /* result = Jenkins32BitMix(Key);*/
    result = (*Wang32BitMixFp)(Key); 
    //result = Wang32BitMix(Key); 

    /*------------------------------------------------------------------*/
    /* Scale the result of the hash down to the size of the hash table. */
    /*------------------------------------------------------------------*/

    /*Try not to use floating point computations.... Rahul */

    //result = ((float)result / ULONG_MAX) * EXT_STA_TABLE_SIZE;
    result = result & (EXT_STA_TABLE_SIZE - 1);// = result%EXT_STA_TABLE_SIZE;
    return(result);
}


/******************************************************************************
*
* Name: LocateAddr
*
* Description:
*    This routine attempts to locate a given MAC address in the external
*    station table. If the MAC address is not currently stored in the table,
*    the pointer returned indicates where it can be added.
*
* Conditions For Use:
*    External station table has been initialized.
*
* Arguments:
*    Arg1 (i  ): Addr_p - Pointer to the MAC address of the station to be
*                         located
*    Arg2 (  o): Item_p - Pointer to the structure in the external station
*                         table if a station with the supplied MAC address
*                         has been found; otherwise, it points to a location
*                         in the table where a station with the given MAC
*                         address can be added
*
* Return Value:
*    Status indicating the results of the operation; possbile values are:
*
*       LOCATE_FAILURE
*       LOCATE_SUCCESS
*
* Notes:
*    If the routine fails to locate a station with the given MAC address in
*    the table, the pointer returned can be used to indicate where to insert
*    the new station; if the pointer is NULL, it means there are no stations
*    in the location resulting from hashing on the MAC address, so the
*    station can be placed at that location. If the pointer is not NULL,
*    then other stations have been placed at the same location (a collision
*    has occurred). In this case the station can be added to a list built
*    up for stations mapped to that location - specifically, the station can
*    be placed on the list by adding it after the structure pointed to by the
*    pointer that is returned.
*
* PDL:
*    Extract the lower 32 bits of the MAC address to be used for hashing
*    Call Hash to get an index into the external station table
*    Search through the list (if one exists) for the given MAC address
*    If found Then
*       Return a pointer to the structure and return success status as well
*    Else
*       Return a pointer to where the station can be added and return
*          failure status as well
*    End If
* END PDL
*
*****************************************************************************/
static extStaDb_Status_e LocateAddr(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                    ExtStaInfoItem_t **Item_pp,
                                    UINT32 *Idx_p )
{
    UINT32 key;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return LOCATE_FAILURE;        
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    /*---------------------------------------------------------------------*/
    /* First, get the lower 32 bits of the MAC address to use for hashing. */
    /*---------------------------------------------------------------------*/
    memcpy(&key,
        ((IEEEtypes_Addr_t *)Addr_p + 2*sizeof(IEEEtypes_Addr_t)),
        4*sizeof(IEEEtypes_Addr_t));
    /*
    if (!((UINT32)Addr_p & 0x3))//Check if Addr_p is at a 4 byte boundary
    {//Addr_p is at 4 byte boundary.
    key = (*Addr_p)[2] | ((*Addr_p)[3] << 8) | ((*Addr_p)[4] << 16) | ((*Addr_p)[5] << 24);
    }
    else
    {//Addr_p+2 is at 4 byte boundary
    key = *((UINT32 *)((UINT8 *)Addr_p+2));
    }
    */
    /*-----------------------------------------------------------*/
    /* Next, hash to get an index into the table that stores MAC */
    /* addresses and associated information.                     */
    /*-----------------------------------------------------------*/
    *Idx_p = Hash(key);

    /*-------------------------------------------------*/
    /* Now see if the address is already in the table. */
    /*-------------------------------------------------*/
    *Item_pp = vmacSta_p->StaCtl->ExtStaInfoDb_p[*Idx_p];
    if ( *Item_pp == NULL )
    {
        return(LOCATE_FAILURE);
    }
    else
    {
        if ( !memcmp((*Item_pp)->StaInfo.Addr,
            Addr_p,
            6*sizeof(IEEEtypes_Addr_t)) )
        {
            return(LOCATE_SUCCESS);
        }
        else
        {
            while ( (*Item_pp)->nxt_ht != NULL )
            {
                *Item_pp = (*Item_pp)->nxt_ht;

                if ( !memcmp((*Item_pp)->StaInfo.Addr,
                    Addr_p,
                    6*sizeof(IEEEtypes_Addr_t))  )
                {
                    return(LOCATE_SUCCESS);
                }
            }
        }
    }

    return(LOCATE_FAILURE);
}

/*!
* Stations in station database
* @param none
* @return number of stations      
*/
extern UINT16 extStaDb_entries(vmacApInfo_t *vmac_p, UINT8 flag )
{
    ListItem *search;
    ExtStaInfoItem_t *search1;
    UINT16 StaList_entries= 0;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(0);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);
#ifdef NPROTECTION
    CleanCounterClient(vmac_p);
#endif
    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/

    search = vmacSta_p->StaCtl->StaList.head;
    while ( search )
    {
        search1 = (ExtStaInfoItem_t *)search;
#ifdef APCFGUR
        if(flag==1)
        {
            if(search1->StaInfo.UR &&  (search1->StaInfo.State == ASSOCIATED))
                StaList_entries++;
        }
        else 
#endif
#ifdef PPPoE_SUPPORT
            if(flag==2)
            {
                if ( ((search1->StaInfo.AP == FALSE) && ((search1->StaInfo.State == ASSOCIATED))) || search1->StaInfo.AP)
                {
                    StaList_entries++;
                }
            }
            else
#endif
                if ( (search1->StaInfo.AP == FALSE) && ((search1->StaInfo.State == AUTHENTICATED) || (search1->StaInfo.State == ASSOCIATED)))
                {
                    if(!memcmp(&vmac_p->macBssId, &search1->StaInfo.Bssid,sizeof(IEEEtypes_MacAddr_t)))
                        StaList_entries++;
#ifdef NPROTECTION
                    ClientStatistics(vmac_p, &search1->StaInfo);
#endif                  
                }
                search = search->nxt;
    }

    os_SemaphorePut(staSH);

    return(StaList_entries);

}

/*!
* list all stations' mac addresses to buf
* @param buf Pointer to station list data structure
* @return OS_SUCCESS
*         OS_FAIL      
*/
extern UINT16 extStaDb_list(vmacApInfo_t *vmac_p, UINT8 *buf, UINT8 get )
{
    int j;
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == buf)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return (0);
    }
    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/

    j = 0;

    search = vmacSta_p->StaCtl->StaList.head;
    while (search)
    {
        search1 = (ExtStaInfoItem_t *)search;
#ifdef PPPoE_SUPPORT
        if (get == 3)
        {
            if((search1->StaInfo.AP == FALSE &&  (search1->StaInfo.State == ASSOCIATED)) || search1->StaInfo.AP)
            {
                UINT32 *ptr;
                ptr = (UINT32 *)&buf[j];
                *ptr = (UINT32)&search1->StaInfo;
                j += 4;
            }
        }
        else
#endif      
#ifdef APCFGUR
            if(get == 2)
            {
                if(search1->StaInfo.UR &&  (search1->StaInfo.State == ASSOCIATED))
                {
                    UINT32 *ptr;
                    ptr = (UINT32 *)&buf[j];
                    *ptr = (UINT32)&search1->StaInfo;
                    j += 4;
                }
            }
            else
#endif
                if ( (search1->StaInfo.AP == FALSE) && (
                    (search1->StaInfo.State == AUTHENTICATED) ||  //removed for now JS
                    (search1->StaInfo.State == ASSOCIATED))
                    && (!memcmp(&vmac_p->macBssId, &search1->StaInfo.Bssid,sizeof(IEEEtypes_MacAddr_t))))
                {
                    /*station mac address */
                    buf[j++] = search1->StaInfo.Addr[0];
                    buf[j++] = search1->StaInfo.Addr[1];
                    buf[j++] = search1->StaInfo.Addr[2];
                    buf[j++] = search1->StaInfo.Addr[3];
                    buf[j++] = search1->StaInfo.Addr[4];
                    buf[j++] = search1->StaInfo.Addr[5];
#ifndef STA_INFO_DB            
                    buf[j++] = 0;
#endif
                    /*station association state */
                    if (search1->StaInfo.State == ASSOCIATED)
                        buf[j++] = TRUE;
                    else
                        buf[j++] = FALSE;

#ifdef STA_INFO_DB
                    buf[j++] = search1->StaInfo.ClientMode;
                    buf[j++] = search1->StaInfo.Rate;
                    buf[j++] = search1->StaInfo.Sq2;
                    buf[j++] = search1->StaInfo.Sq1;
                    buf[j++] = search1->StaInfo.RSSI;
                    if ( search1->StaInfo.PwrMode == PWR_MODE_PWR_SAVE )
                        buf[j++] = TRUE;
                    else
                        buf[j++] = FALSE;
#endif
                }
                search = search->nxt;
    }

    os_SemaphorePut(staSH);

    return (1);

}

extern UINT16 extStaDb_AggrCk( vmacApInfo_t *vmac_p)
{
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;
    Aggr11n *pAggr11n;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return (0);
    }
    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    search = vmacSta_p->StaCtl->StaList.head;
    while (search)
    {
        search1 = (ExtStaInfoItem_t *)search;
        pAggr11n = &search1->StaInfo.aggr11n;
#ifndef AMSDUOVERAMPDU
        if((pAggr11n->type&WL_WLAN_TYPE_AMSDU) && (*(search1->StaInfo.mib_p->pMib_11nAggrMode)&WL_MODE_AMSDU_TX_MASK))
        {
            if (pAggr11n->threshold && (pAggr11n->txcnt> pAggr11n->threshold))
            {
                pAggr11n->start = AGGKEEPNUM;
            }
            else
            {
                if (pAggr11n->start != 0)
                    pAggr11n->start--;
            }
        }
#endif
#ifdef AMPDU_SUPPORT
        {
            int tid;
            if(*(search1->StaInfo.mib_p->mib_AmpduTx)==3)
                for (tid =0; tid <7 ; tid++)
                {

                    if (pAggr11n->threshold && (pAggr11n->txcntbytid[tid]> pAggr11n->threshold))
                    {
                        pAggr11n->onbytid[tid]=1;
                    }
                    else
                    {
                        if(pAggr11n->threshold && (pAggr11n->txcntbytid[tid]< 4))
                        {
                            //disableAmpduTx(vmacSta_p,search1->StaInfo.Addr, tid);
                            pAggr11n->onbytid[tid]=0;
                            //search1->StaInfo.aggr11n.startbytid[tid]=0;
                        }
                    }
                    pAggr11n->txcntbytid[tid] = 0;
                }
        }
#endif
        pAggr11n->txcnt =0;
        search = search->nxt;
    }
    os_SemaphorePut(staSH);
    return (1);
}
extern UINT16 extStaDb_AggrFrameCk(vmacApInfo_t *vmac_p, int force)
{
    int retval = 0;
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    //if(!( vmac_p->dev->flags & IFF_RUNNING))
    //  return 0;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return (0);
    }
    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    search = vmacSta_p->StaCtl->StaList.head;
    while (search)
    {
        search1 = (ExtStaInfoItem_t *)search;
        if (*(search1->StaInfo.mib_p->QoSOptImpl)&& (search1->StaInfo.aggr11n.type&WL_WLAN_TYPE_AMSDU))//search1->StaInfo.aggr11n.threshold)
        {
#ifdef WDS_FEATURE
            struct net_device *dev = (struct net_device *)(search1->StaInfo.wdsInfo);
            if (dev) 
                retval = send_11n_aggregation_skb(dev, &search1->StaInfo, force);
            else
#endif
                retval = send_11n_aggregation_skb(search1->StaInfo.dev, &search1->StaInfo, force);
        }
        search = search->nxt;
    }
    os_SemaphorePut(staSH);
    return retval;
}



/*This function cleans up and send deauth msg to sta*/
extern extStaDb_Status_e extStaDb_RemoveStaNSendDeauthMsg(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p)
{
	extStaDb_StaInfo_t *StaInfo_p;
	vmacApInfo_t *vmacSta_p;
	
	if(vmac_p->master)
		vmacSta_p= vmac_p->master;
	else
		vmacSta_p = vmac_p;

	if ( (StaInfo_p = extStaDb_GetStaInfo(vmac_p,Addr_p, 0)) == NULL )
	{
		/* Station not known, do nothing */
		return LOCATE_FAILURE;
	}
	if (StaInfo_p->AP == TRUE)
	{
		return LOCATE_FAILURE;
	}

	
#ifdef AP_URPTR
	/* Clean up UR hash table for this STA */
	urHash_RemoveSta(vmacSta_p,Addr_p, 0);
#endif
	if ( StaInfo_p->State == ASSOCIATED )
	{
#ifdef AMPDU_SUPPORT
		cleanupAmpduTx(vmac_p,(UINT8 *)Addr_p);
#endif
		macMgmtMlme_SendDeauthenticateMsg(vmac_p,Addr_p, StaInfo_p->StnId,IEEEtypes_REASON_DISASSOC_INACTIVE );
		FreeAid(StaInfo_p->Aid);
		ResetAid(StaInfo_p->StnId, StaInfo_p->Aid);
		StaInfo_p->Aid = 0;
	}
	if (StaInfo_p->PwrMode == PWR_MODE_PWR_SAVE)
	{
		if (vmac_p->PwrSaveStnCnt)
			vmac_p->PwrSaveStnCnt--;
		StaInfo_p->PwrMode= PWR_MODE_ACTIVE;
	}
	if (StaInfo_p->ClientMode == BONLY_MODE)
		macMgmtMlme_DecrBonlyStnCnt(vmac_p,0);
	
	
	if (StaInfo_p->Aid !=0)			
	{
		FreeAid(StaInfo_p->Aid);
		ResetAid(StaInfo_p->StnId, StaInfo_p->Aid);
		StaInfo_p->Aid = 0;
	}
	StaInfo_p->State = UNAUTHENTICATED;

	extStaDb_DelSta(vmac_p,Addr_p,0);
	FreeStnId(StaInfo_p->StnId);
	
	return(DEL_SUCCESS);
}

/*!
* Remove a station from the station database plus cleanup
* @param Addr_p Pointer to the station mac address
* @return DEL_SUCCESS,      
*         LOCATE_FAILURE          
*/
extern extStaDb_Status_e extStaDb_RemoveSta(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p)
{
    extStaDb_StaInfo_t *StaInfo_p;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return LOCATE_FAILURE;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if ( (StaInfo_p = extStaDb_GetStaInfo(vmac_p,Addr_p, 0)) == NULL )
    {
        /* Station not known, do nothing */
        return LOCATE_FAILURE;
    }
    if (StaInfo_p->AP == TRUE)
    {
        return LOCATE_FAILURE;
    }
#ifdef AP_URPTR
    /* Clean up UR hash table for this STA */
    urHash_RemoveSta(vmacSta_p,Addr_p, 1);
#endif
    if ( StaInfo_p->State == ASSOCIATED )
    {

        macMgmtCleanUp(vmac_p,StaInfo_p);


    }
    StaInfo_p->State = UNAUTHENTICATED;
    FreeStnId(StaInfo_p->StnId);
    extStaDb_DelSta(vmac_p,Addr_p, 0);
    return(DEL_SUCCESS);

}

UINT8 GetAssociateMacAddress(vmacApInfo_t *vmac_p,unsigned char *MacAddress)
{
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == MacAddress)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if ( !vmacSta_p->StaCtl->Initialized )
    {
        return(0);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/

    search = vmacSta_p->StaCtl->StaList.head;
    while ( search )
    {
        search1 = (ExtStaInfoItem_t *)search;
        if ( (search1->StaInfo.State == ASSOCIATED) )
        {
            memcpy(MacAddress, search1->StaInfo.Addr, 6);
            os_SemaphorePut(staSH);
            return 1;
        }
        search = search->nxt;
    }

    os_SemaphorePut(staSH);

    return 0;

}

void extStaDb_SetNewState4AllSta(vmacApInfo_t *vmac_p,extStaDb_State_e NewState)
{
    vmacApInfo_t *vmacSta_p;
    extStaDb_StaInfo_t *StaInfo_p;
    ExtStaInfoItem_t *Curr_p, *Item_p;

    if (NULL == vmac_p)
        return;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
    while ( Curr_p != NULL )
    {
        Item_p = Curr_p;
        if ( (StaInfo_p = extStaDb_GetStaInfo(vmac_p,&(Item_p->StaInfo.Addr), 0)))
        {
            if(StaInfo_p->State == ASSOCIATED)
            {
                StaInfo_p->State = NewState;
                if (NewState != ASSOCIATED)
                {
                    StaInfo_p->keyMgmtStateInfo.RSNDataTrafficEnabled = 0;
                }
                if (NewState == AUTHENTICATED)
                {
                    FreeAid(StaInfo_p->Aid);
                    StaInfo_p->Aid = 0;
                }
#ifdef QOS_FEATURE
                if(NewState == UNAUTHENTICATED )
                {
                    StaInfo_p->IsStaQSTA = FALSE;
                }
#endif
            }
        }
        Curr_p = Curr_p->nxt;
    }
    
    return;
}
void extStaDb_SendGrpKeyMsgToAllSta(vmacApInfo_t *vmac_p)
{
    MhsmEvent_t msg;
    vmacApInfo_t *vmacSta_p;
    extStaDb_StaInfo_t *StaInfo_p;
    ExtStaInfoItem_t *Curr_p, *Item_p;

    if (NULL == vmac_p)
        return;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    Curr_p = (ExtStaInfoItem_t *)(vmacSta_p->StaCtl->StaList.head);
    while ( Curr_p != NULL )
    {
        Item_p = Curr_p;
        if ( (StaInfo_p = extStaDb_GetStaInfo(vmac_p,&(Item_p->StaInfo.Addr), 0)))
        {
            if(StaInfo_p->State == ASSOCIATED)
            {
        msg.event = GRPKEYTIMEOUT_EVT;
#ifdef AP_MAC_LINUX
        msg.devinfo = (void *) vmac_p;
#endif
                ProcessKeyMgmtData(vmac_p,NULL, &StaInfo_p->Addr, &msg);
            }
        }
        Curr_p = Curr_p->nxt;
    }

    return;
}

extern extStaDb_Status_e extStaDb_SetRSNDataTrafficEnabled(vmacApInfo_t *vmac_p,
                                                           IEEEtypes_MacAddr_t *Addr_p, UINT8 value )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    //    item_p->StaInfo.keyMgmtStateInfo->RSNDataTrafficEnabled = value;
    item_p->StaInfo.keyMgmtStateInfo.RSNDataTrafficEnabled = value;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extStaDb_Status_e extStaDb_SetRSNPwkAndDataTraffic(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                                   UINT8* pEncryptKey, UINT32* pTxMICKey, UINT32* pRxMICKey)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pEncryptKey || NULL == pTxMICKey || NULL == pRxMICKey)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    /*    item_p->StaInfo.keyMgmtStateInfo->RSNDataTrafficEnabled = TRUE;
    memcpy(item_p->StaInfo.keyMgmtStateInfo->u.RSNPwkEncryptKey, pEncryptKey, 16);
    memcpy(item_p->StaInfo.keyMgmtStateInfo->RSNPwkTxMICKey, pTxMICKey, 8);
    memcpy(item_p->StaInfo.keyMgmtStateInfo->RSNPwkRxMICKey, pRxMICKey, 8);
    */
    item_p->StaInfo.keyMgmtStateInfo.RSNDataTrafficEnabled = TRUE;
    //memcpy(item_p->StaInfo.keyMgmtStateInfo.tk1.RSNPwkEncryptKey, pEncryptKey, 16);
    memcpy(item_p->StaInfo.keyMgmtStateInfo.PairwiseTempKey1, pEncryptKey, 16);
    memcpy(item_p->StaInfo.keyMgmtStateInfo.RSNPwkTxMICKey, pTxMICKey, 8);
    memcpy(item_p->StaInfo.keyMgmtStateInfo.RSNPwkRxMICKey, pRxMICKey, 8);
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    //#endif
    return(STATE_SUCCESS);
}



extern extStaDb_Status_e extStaDb_SetRSNPwk(vmacApInfo_t *vmac_p,
                                            IEEEtypes_MacAddr_t *Addr_p,
                                            UINT8* pEncryptKey, UINT32* pTxMICKey, UINT32* pRxMICKey )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pEncryptKey || NULL == pTxMICKey || NULL == pRxMICKey)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    memcpy(item_p->StaInfo.keyMgmtStateInfo.PairwiseTempKey1, pEncryptKey, 16);
    memcpy(item_p->StaInfo.keyMgmtStateInfo.RSNPwkTxMICKey, pTxMICKey, 8);
    memcpy(item_p->StaInfo.keyMgmtStateInfo.RSNPwkRxMICKey, pRxMICKey, 8);
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extern extStaDb_Status_e extStaDb_SetRSNPmk(vmacApInfo_t *vmac_p,
                                            IEEEtypes_MacAddr_t *Addr_p, UINT8* pPMK )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pPMK)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    memcpy(item_p->StaInfo.keyMgmtStateInfo.PMK, pPMK, 32);
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extern extStaDb_Status_e extStaDb_GetRSN_IE( vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                            UINT8 *RsnIE_p )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == RsnIE_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Get the value. */
    /*------------------------*/
    memcpy((UINT8*)RsnIE_p, item_p->StaInfo.keyMgmtStateInfo.RsnIEBuf,
        2 + *(item_p->StaInfo.keyMgmtStateInfo.RsnIEBuf + 1));
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

#ifdef MRVL_WSC //MRVL_WSC_IE
extern extStaDb_Status_e extStaDb_GetWSC_IE( vmacApInfo_t *vmac_p,  IEEEtypes_MacAddr_t *Addr_p,
                                            UINT8 *WscIE_p )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == WscIE_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Get the value. */
    /*------------------------*/
    memcpy((UINT8*)WscIE_p, &item_p->StaInfo.WscIEBuf,
        2 + item_p->StaInfo.WscIEBuf.Len);
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}
#endif //WSC_MRVL_IE


extern extStaDb_Status_e extStaDb_SetRSN_IE(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                            IEEEtypes_RSN_IE_t *RsnIE_p )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    UINT8 RsnIELen;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == RsnIE_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    RsnIELen = *((UINT8 *)RsnIE_p + 1);
    if ( (RsnIELen + 2) <= MAX_SIZE_RSN_IE_BUF )
    {
        memset(item_p->StaInfo.keyMgmtStateInfo.RsnIEBuf, 0, MAX_SIZE_RSN_IE_BUF);
        memcpy((UINT8*)item_p->StaInfo.keyMgmtStateInfo.RsnIEBuf, RsnIE_p, RsnIELen + 2);
    }
    else
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(RSN_IE_BUF_OVERFLOW);
    }

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extern extStaDb_Status_e extStaDb_GetPairwiseTSC(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                                 UINT32 *pTxIV32, UINT16 *pTxIV16 )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pTxIV32 || NULL == pTxIV16)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Get the value. */
    /*------------------------*/
    *pTxIV32 = item_p->StaInfo.keyMgmtStateInfo.TxIV32;
    *pTxIV16 = item_p->StaInfo.keyMgmtStateInfo.TxIV16;
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extern extStaDb_Status_e extStaDb_SetPairwiseTSC(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                                 UINT32 TxIV32, UINT16 TxIV16 )
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Get the value. */
    /*------------------------*/
    item_p->StaInfo.keyMgmtStateInfo.TxIV32 = TxIV32;
    item_p->StaInfo.keyMgmtStateInfo.TxIV16 = TxIV16;
    item_p->StaInfo.keyMgmtStateInfo.RxIV32 = 0xFFFFFFFF; //Rahul
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

extStaDb_Status_e extStaDb_GetStaInfoAndTxKeys( vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                               extStaDb_StaInfo_t *StaInfo_p, UINT32 AgingTimeMode)
{
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == StaInfo_p)
        return LOCATE_FAILURE;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);         //masked by laser for GST
#endif

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /* if not found, give back the semaphore and return error status.   */
    /*------------------------------------------------------------------*/
    if (LocateAddr(vmacSta_p,Addr_p, &item_p, &idx) == LOCATE_SUCCESS)
    {
        /*-------------------------------------------*/
        /* Fill out the requested state information. */
        /*-------------------------------------------*/
        //*StaInfo_p = item_p->StaInfo;
        memcpy(StaInfo_p, &item_p->StaInfo,
            sizeof(extStaDb_StaInfo_t) - sizeof(keyMgmtInfo_t));

        memcpy(StaInfo_p->keyMgmtStateInfo.PairwiseTempKey1,
            item_p->StaInfo.keyMgmtStateInfo.PairwiseTempKey1, 16);
        memcpy(StaInfo_p->keyMgmtStateInfo.RSNPwkTxMICKey,
            item_p->StaInfo.keyMgmtStateInfo.RSNPwkTxMICKey, 8);
        StaInfo_p->keyMgmtStateInfo.TxIV32 =
            item_p->StaInfo.keyMgmtStateInfo.TxIV32;
        StaInfo_p->keyMgmtStateInfo.TxIV16 =
            item_p->StaInfo.keyMgmtStateInfo.TxIV16;
        memcpy(StaInfo_p->keyMgmtStateInfo.Phase1KeyTx,
            item_p->StaInfo.keyMgmtStateInfo.Phase1KeyTx, 10);
        StaInfo_p->QueueToUse=item_p->StaInfo.QueueToUse;
        StaInfo_p->keyMgmtStateInfo.RSNDataTrafficEnabled =
            item_p->StaInfo.keyMgmtStateInfo.RSNDataTrafficEnabled;
        memcpy(StaInfo_p->keyMgmtStateInfo.RsnIEBuf,
            item_p->StaInfo.keyMgmtStateInfo.RsnIEBuf, MAX_SIZE_RSN_IE_BUF);
        //StaInfo_p->ClientMode=item_p->StaInfo.ClientMode;
        //StaInfo_p->ApMode = item_p->StaInfo.ApMode;
        //Increment the counters
        item_p->StaInfo.keyMgmtStateInfo.TxIV16++;
        if (item_p->StaInfo.keyMgmtStateInfo.TxIV16 == 0)
        {
            item_p->StaInfo.keyMgmtStateInfo.TxIV32++;
        }
        StaInfo_p->QueueToUse=item_p->StaInfo.QueueToUse;
        switch (AgingTimeMode)
        {
        case 0:
            break;
        case 1:
            item_p->StaInfo.TimeStamp = vmacSta_p->StaCtl->aging_time_in_minutes;  // AGING_TIME;
        case 2:
            if (item_p->StaInfo.TimeStamp > 2)
                item_p->StaInfo.TimeStamp = 2;  // minimum unit
            break;
        default:
            break;
        }
        /*-------------------------------------------------------*/
        /* Finished - give back the semaphore and return status. */
        /*-------------------------------------------------------*/
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);   //masked by laser for GST
#endif
        return(STATE_SUCCESS);
    }
    else
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return LOCATE_FAILURE;
    }
}


extStaDb_Status_e extStaDb_GetRSNPwk(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                     UINT8* pEncryptKey, UINT32* pTxMICKey, UINT32* pRxMICKey)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pEncryptKey || NULL == pTxMICKey || NULL == pRxMICKey)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);   //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    /*    memcpy(pEncryptKey, item_p->StaInfo.keyMgmtStateInfo->u.RSNPwkEncryptKey, 16);
    memcpy(pTxMICKey, item_p->StaInfo.keyMgmtStateInfo->RSNPwkTxMICKey, 8);
    memcpy(pRxMICKey, item_p->StaInfo.keyMgmtStateInfo->RSNPwkRxMICKey, 8);
    */
    //memcpy(pEncryptKey, item_p->StaInfo.keyMgmtStateInfo.tk1.RSNPwkEncryptKey, 16);
    memcpy(pEncryptKey, item_p->StaInfo.keyMgmtStateInfo.PairwiseTempKey1, 16);
    memcpy(pTxMICKey, item_p->StaInfo.keyMgmtStateInfo.RSNPwkTxMICKey, 8);
    memcpy(pRxMICKey, item_p->StaInfo.keyMgmtStateInfo.RSNPwkRxMICKey, 8);
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);   //masked by laser
#endif
    return(STATE_SUCCESS);
}

extStaDb_Status_e extStaDb_GetStaInfoAndRxKeys(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                               extStaDb_StaInfo_t *StaInfo_p, UINT32 AgingTimeMode)
{
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == StaInfo_p)
        return LOCATE_FAILURE;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);   //masked laser for GST
#endif 

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /* if not found, give back the semaphore and return error status.   */
    /*------------------------------------------------------------------*/
    if (LocateAddr(vmacSta_p,Addr_p, &item_p, &idx) == LOCATE_SUCCESS)
    {
        /*-------------------------------------------*/
        /* Fill out the requested state information. */
        /*-------------------------------------------*/
        //*StaInfo_p = item_p->StaInfo;
        memcpy(StaInfo_p, &item_p->StaInfo,
            sizeof(extStaDb_StaInfo_t) - sizeof(keyMgmtInfo_t));

        memcpy(StaInfo_p->keyMgmtStateInfo.PairwiseTempKey1,
            item_p->StaInfo.keyMgmtStateInfo.PairwiseTempKey1, 16);

        memcpy(StaInfo_p->keyMgmtStateInfo.RSNPwkRxMICKey,
            item_p->StaInfo.keyMgmtStateInfo.RSNPwkRxMICKey, 8);
        memcpy(StaInfo_p->keyMgmtStateInfo.Phase1KeyRx,
            item_p->StaInfo.keyMgmtStateInfo.Phase1KeyRx, 10);
        StaInfo_p->keyMgmtStateInfo.RSNDataTrafficEnabled =
            item_p->StaInfo.keyMgmtStateInfo.RSNDataTrafficEnabled;
        StaInfo_p->keyMgmtStateInfo.RxIV32 = 
            item_p->StaInfo.keyMgmtStateInfo.RxIV32;

        if (AgingTimeMode == 2)
        {       // mimimum set aging time
            if (item_p->StaInfo.TimeStamp > 2)
                item_p->StaInfo.TimeStamp = 2;  // minimum unit
        }
        else if (AgingTimeMode == 1)
        {   // maxmum set aging time
            if (StaInfo_p->ClientMode == BONLY_MODE)
            {
                item_p->StaInfo.TimeStamp = vmacSta_p->StaCtl->aging_time_in_minutes;  // AGING_TIME;
            }
            else
            {
                item_p->StaInfo.TimeStamp = vmacSta_p->StaCtl->aging_time_in_minutes * 15; 
            }
        }
        /*-------------------------------------------------------*/
        /* Finished - give back the semaphore and return status. */
        /*-------------------------------------------------------*/
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);   //masked by laser for GST
#endif
        return(STATE_SUCCESS);
    }
    else
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
#endif
        return LOCATE_FAILURE;
    }
}


extStaDb_Status_e extStaDb_GetKeyMgmtInfo(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                          keyMgmtInfo_t *KeyMgmtInfo)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == KeyMgmtInfo)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    memcpy(KeyMgmtInfo, &(item_p->StaInfo.keyMgmtStateInfo),
        sizeof(keyMgmtInfo_t));

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
    return(STATE_SUCCESS);
}

extStaDb_Status_e extStaDb_SetKeyMgmtInfo(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                          keyMgmtInfo_t *pKeyMgmtInfo)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pKeyMgmtInfo)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    memcpy(&(item_p->StaInfo.keyMgmtStateInfo), pKeyMgmtInfo,
        sizeof(keyMgmtInfo_t));

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}

#ifdef QOS_FEATURE

UINT8 extStaDb_GetQoSOptn(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    UINT8 QosOptn;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        //     return(NOT_INITIALIZED);
        return(0); //return 0 for now, should not matter for caller of this function. original function not well written
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        //  return(result);
        return(0); //return 0 for now, should not matter for caller of this function. original function not well written
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    QosOptn = item_p->StaInfo.IsStaQSTA;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
    //    return(STATE_SUCCESS);
    return(QosOptn);

}

extern extStaDb_Status_e extStaDb_SetQoSOptn(vmacApInfo_t *vmac_p,
                                             IEEEtypes_MacAddr_t *Addr_p,
                                             UINT8 QosOptn)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    item_p->StaInfo.IsStaQSTA = QosOptn;
    
    if(QosOptn != 0)
        item_p->StaInfo.ni_flags |= IEEE80211_NODE_QOS;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
    return(STATE_SUCCESS);
}

#endif


#ifdef WMM_PS_SUPPORT
extern extStaDb_Status_e extStaDb_SetQosInfo (vmacApInfo_t *vmac_p,
                                              IEEEtypes_MacAddr_t *Addr_p,
                                              QoS_WmeInfo_Info_t  *pQosinfo)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == pQosinfo)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    item_p->StaInfo.Qosinfo=  *pQosinfo;

    /** when a u-apsd flag is set to 1, it indicates that the corresponding AC is both a delivery-enabled AC and triggered
    enabled AC **/


    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
    return(STATE_SUCCESS);
}

extern extStaDb_Status_e extStaDb_GetWMM_DeliveryEnableInfo(vmacApInfo_t *vmac_p,
                                                            IEEEtypes_MacAddr_t *Addr_p,
                                                            UINT8 Priority, UINT8 *Wmm_DeliveryInfo, UINT8 *UpdateTim)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == Wmm_DeliveryInfo || NULL == UpdateTim)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST

    *Wmm_DeliveryInfo=0;
    *UpdateTim=0;
    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    switch(AccCategoryQ[Priority])
    {
        /** BG traffic **/
    case 0: if(item_p->StaInfo.Qosinfo.Uapsd_ac_be)
                *Wmm_DeliveryInfo=1;
        break;
    case 1:if(item_p->StaInfo.Qosinfo.Uapsd_ac_bk)
               *Wmm_DeliveryInfo=1;
        break;
    case 2:if(item_p->StaInfo.Qosinfo.Uapsd_ac_vi)
               *Wmm_DeliveryInfo=1;
        break;
    case 3:if(item_p->StaInfo.Qosinfo.Uapsd_ac_vo)
               *Wmm_DeliveryInfo=1;
        break;
    }

    /** 3.6.1.4 In case all ACs are delivery-enabled ACs, WMM AP with the U-APSD bit7 set to 1 in the
    Qos Info Field shall assembled the Partial Virtual Bitmap containing the buffer status for all AC
    per destination for WMM Sta **/
    if((item_p->StaInfo.Qosinfo.Uapsd_ac_be && item_p->StaInfo.Qosinfo.Uapsd_ac_bk
        && item_p->StaInfo.Qosinfo.Uapsd_ac_vi && item_p->StaInfo.Qosinfo.Uapsd_ac_vo) || *Wmm_DeliveryInfo==0)
        *UpdateTim=1;


    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
    return(STATE_SUCCESS);
}

extern UINT8 extStaDb_Check_Uapsd_Capability(vmacApInfo_t *vmac_p,
                                             IEEEtypes_MacAddr_t *Addr_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return 1;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(1);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        return(1);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/

    /** 3.6.1.4 In case all ACs are delivery-enabled ACs, WMM AP with the U-APSD bit7 set to 1 in the
    Qos Info Field shall assembled the Partial Virtual Bitmap containing the buffer status for all AC
    per destination for WMM Sta **/
    if((item_p->StaInfo.Qosinfo.Uapsd_ac_be || item_p->StaInfo.Qosinfo.Uapsd_ac_bk
        || item_p->StaInfo.Qosinfo.Uapsd_ac_vi || item_p->StaInfo.Qosinfo.Uapsd_ac_vo) )
    {
        return 1;
    }


    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    return(0);  /** not all ac are delivery enable **/
}


extern UINT8 extStaDb_Check_ALL_AC_DeliveryEnableInfo(vmacApInfo_t *vmac_p,
                                                      IEEEtypes_MacAddr_t *Addr_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return 1;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(1);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        return(1);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/

    /** 3.6.1.4 In case all ACs are delivery-enabled ACs, WMM AP with the U-APSD bit7 set to 1 in the
    Qos Info Field shall assembled the Partial Virtual Bitmap containing the buffer status for all AC
    per destination for WMM Sta **/
    if((item_p->StaInfo.Qosinfo.Uapsd_ac_be && item_p->StaInfo.Qosinfo.Uapsd_ac_bk
        && item_p->StaInfo.Qosinfo.Uapsd_ac_vi && item_p->StaInfo.Qosinfo.Uapsd_ac_vo) )
    {
        return 1;
    }


    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    return(0);  /** not all ac are delivery enable **/
}

#endif

extStaDb_Status_e extStaDb_SetPhase1Key(vmacApInfo_t *vmac_p,IEEEtypes_MacAddr_t *Addr_p,
                                        UINT16 *Phase1Key,
                                        PacketType_e mode, UINT32 RxIV32)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == Phase1Key)
        return LOCATE_FAILURE;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    if (mode == Tx)
        memcpy(item_p->StaInfo.keyMgmtStateInfo.Phase1KeyTx, Phase1Key, 10);
    //(*my_memcpyFp)(item_p->StaInfo.keyMgmtStateInfo.Phase1KeyTx, Phase1Key, 10);
    else
    {
        memcpy(item_p->StaInfo.keyMgmtStateInfo.Phase1KeyRx, Phase1Key, 10);
        item_p->StaInfo.keyMgmtStateInfo.RxIV32 = RxIV32;
    }
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
    return(STATE_SUCCESS);
}


int StnMacAddressCopy(vmacApInfo_t *vmac_p,unsigned char *tempmac)
{
    int i = 0;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == tempmac)
        return -1;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    for (i = 0;i < 32;i++)
    {
        memcpy((tempmac + i*6), vmacSta_p->StaCtl->ExtStaInfoDb[i].StaInfo.Addr, 6);

        if (memcmp(tempmac + i*6, "\x00\x00\x00\x00\x00\x00", 6) == 0)
            return i;


    }
    return i;

}
extern extStaDb_Status_e extStaDb_UpdateAgingTime(vmacApInfo_t *vmac_p,
                                                  IEEEtypes_MacAddr_t *Addr_p)
{
#if 1
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        os_SemaphorePut(sysinfo_EXT_STA_SEM);
        return(result);
    }

    /*------------------------*/
    /* Update the power mode. */
    /*------------------------*/
    item_p->StaInfo.TimeStamp = vmacSta_p->StaCtl->aging_time_in_minutes  * 15;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    os_SemaphorePut(sysinfo_EXT_STA_SEM);
#endif
    return(STATE_SUCCESS);
}

extStaDb_Status_e extStaDb_GetStaInfo_noWait(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                             extStaDb_StaInfo_t *StaInfo_p, UINT32 AgingTimeMode)
{
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == StaInfo_p)
        return LOCATE_FAILURE;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    //os_SemaphoreGet(sysinfo_EXT_STA_SEM);         //masked by laser for GST
#endif

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /* if not found, give back the semaphore and return error status.   */
    /*------------------------------------------------------------------*/
    if (LocateAddr(vmacSta_p,Addr_p, &item_p, &idx) == LOCATE_SUCCESS)
    {
        /*-------------------------------------------*/
        /* Fill out the requested state information. */
        /*-------------------------------------------*/
        //*StaInfo_p = item_p->StaInfo;

        StaInfo_p->QueueToUse=item_p->StaInfo.QueueToUse;
        StaInfo_p->IsStaQSTA=item_p->StaInfo.IsStaQSTA;
        StaInfo_p->Aid=item_p->StaInfo.Aid;

        /*-------------------------------------------------------*/
        /* Finished - give back the semaphore and return status. */
        /*-------------------------------------------------------*/
#ifndef AP32GST
        // os_SemaphorePut(sysinfo_EXT_STA_SEM);   //masked by laser for GST
#endif
        return(STATE_SUCCESS);
    }
    else
    {
#ifndef AP32GST
        // os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return LOCATE_FAILURE;
    }
}

/********** zhouke add ,set auth aging time **********/
extern int set_sta_auth_aging_time(vmacApInfo_t *vmac_p,int minutes)
{
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;

    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (NULL == vmacSta_p)
        return 0;
    
    vmacSta_p->auth_time_in_minutes= minutes;
    return minutes;
}
/*************** end *************************/

extern int set_sta_aging_time(vmacApInfo_t *vmac_p,int minutes)
{
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;

    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (NULL == vmacSta_p)
        return 0;
    
    vmacSta_p->StaCtl->aging_time_in_minutes = minutes / AGING_TIMER_VALUE_IN_SECONDS;
    return minutes;
}

#ifdef FLEX_TIME
/** Get AGAssociation cnt **/

void extStaDb_AGStnsCnt(vmacApInfo_t *vmac_p)
{
    int j;
    ListItem *search;
    ExtStaInfoItem_t *search1;
    UINT8 tempAOnlyClientCnt,tempBGOnlyClientCnt;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return (0);
    }
    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/

    j = 0;
    tempAOnlyClientCnt=tempBGOnlyClientCnt=0;
    search = vmacSta_p->StaCtl->StaList.head;
    while (search)
    {
        search1 = (ExtStaInfoItem_t *)search;
        if ( (search1->StaInfo.AP == FALSE) && ( (search1->StaInfo.State == ASSOCIATED)))
        {
            if(search1->StaInfo.ClientMode==AONLY_MODE)
            {
                tempAOnlyClientCnt++;

            }
            else
            {
                tempBGOnlyClientCnt++;

            }
        }
        search = search->nxt;
    }

    AOnlyClientCnt=tempAOnlyClientCnt;
    BGOnlyClientCnt=tempBGOnlyClientCnt;
    os_SemaphorePut(staSH);

    return (1);
}
#endif
#ifdef IEEE80211H
extern extStaDb_Status_e extStaDb_GetMeasurementInfo(vmacApInfo_t *vmac_p,
                                                     IEEEtypes_MacAddr_t *Addr_p,
                                                     extStaDb_measurement_info_t *measureInfo_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == measureInfo_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    *measureInfo_p = item_p->StaInfo.measureInfo;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
    return(STATE_SUCCESS);
}


extern extStaDb_Status_e extStaDb_SetMeasurementInfo(vmacApInfo_t *vmac_p,
                                                     IEEEtypes_MacAddr_t *Addr_p,
                                                     extStaDb_measurement_info_t *measureInfo_p)
{
    extStaDb_Status_e result;
    ExtStaInfoItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p || NULL == Addr_p || NULL == measureInfo_p)
        return NOT_INITIALIZED;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphoreGet(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif

    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be updated; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = LocateAddr(vmacSta_p,Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
#ifndef AP32GST
        os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
        return(result);
    }

    /*------------------------*/
    /* Update the value. */
    /*------------------------*/
    item_p->StaInfo.measureInfo = *measureInfo_p;

    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
#ifndef AP32GST
    os_SemaphorePut(sysinfo_EXT_STA_SEM);  //masked by laser for GST
#endif
    return(STATE_SUCCESS);
}

#endif
#ifdef INTOLERANT40
void extStaDb_SendBeaconReqMeasureReqAction(vmacApInfo_t *vmac_p)
{
    extern BOOLEAN macMgmtMlme_SendBeaconReqMeasureReqAction(struct net_device *dev,IEEEtypes_MacAddr_t *Addr);
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    if (!vmacSta_p->StaCtl->Initialized)
    {
        return (0);
    }

    os_SemaphoreGet(staSH);

    search = vmacSta_p->StaCtl->StaList.head;
    while ( search )
    {
        search1 = (ExtStaInfoItem_t *)search;
        if ( (search1->StaInfo.AP == FALSE) &&(search1->StaInfo.State == ASSOCIATED) && (search1->StaInfo.ClientMode == NONLY_MODE) )
        {           
            macMgmtMlme_SendBeaconReqMeasureReqAction(vmac_p->dev,(IEEEtypes_MacAddr_t *)search1->StaInfo.Addr);           
        }
        search = search->nxt;
    }
    os_SemaphorePut(staSH);
}
#endif

WL_STATUS ethStaDb_Init(vmacApInfo_t *vmacSta_p, UINT16 MaxStns )
{
    UINT32 i;

    if (NULL == vmacSta_p)
        return OS_FAIL;
    if (vmacSta_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA) {
        if(vmacSta_p->master)
            return(OS_SUCCESS);
    }

    if(vmacSta_p->EthStaCtl == NULL)
    {
        vmacSta_p->EthStaCtl= (struct ETHSTADB_CTL *)malloc(sizeof(struct ETHSTADB_CTL));

        if(vmacSta_p->EthStaCtl == NULL)
        {
            printk("fail to alloc memory\n");
            return OS_FAIL;
        }
        memset(vmacSta_p->EthStaCtl, 0, sizeof(struct ETHSTADB_CTL));
    }
    vmacSta_p->EthStaCtl->aging_time_in_minutes = 3*60 / AGING_TIMER_VALUE_IN_SECONDS;

    /*pre-allocate sta datebase */
    if ( vmacSta_p->EthStaCtl->EthStaDb == NULL )
    {
        vmacSta_p->EthStaCtl->EthStaDb = (EthStaItem_t *)malloc(MaxStns *sizeof(EthStaItem_t));  
        if (vmacSta_p->EthStaCtl->EthStaDb == NULL)
        {
            printk("fail to alloc memory\n");
            return OS_FAIL;
        }
        memset(vmacSta_p->EthStaCtl->EthStaDb, 0, MaxStns *sizeof(EthStaItem_t));

    }

    /*----------------------------------------------------------------*/
    /* Initial the array of pointers to station information elements; */
    /* initially, the pointers all point to nothing.                  */
    /*----------------------------------------------------------------*/
    for (i = 0; i < EXT_STA_TABLE_SIZE; i++)
    {
        vmacSta_p->EthStaCtl->EthStaDb_p[i] = NULL;
    }

    ListInit(&vmacSta_p->EthStaCtl->FreeEthStaList);
    ListInit(&vmacSta_p->EthStaCtl->EthStaList);
    /*-------------------------------------------------------------*/
    /* Set up the list of initially free elements that are used to */
    /* record external station information.                        */
    /*-------------------------------------------------------------*/
    for ( i = 0; i < MaxStns; i++ )
    {
        vmacSta_p->EthStaCtl->EthStaDb[i].nxt = NULL;
        vmacSta_p->EthStaCtl->EthStaDb[i].prv = NULL;
        vmacSta_p->EthStaCtl->EthStaDb[i].nxt_ht = NULL;
        vmacSta_p->EthStaCtl->EthStaDb[i].prv_ht = NULL;
        vmacSta_p->EthStaCtl->EthStaDb[i].ethStaInfo.TimeStamp = 30;
        vmacSta_p->EthStaCtl->EthStaDb[i].ethStaInfo.pStaInfo_t = NULL;
        memset(&vmacSta_p->EthStaCtl->EthStaDb[i].ethStaInfo.Addr, 0, sizeof(IEEEtypes_MacAddr_t));
        ListPutItem(&vmacSta_p->EthStaCtl->FreeEthStaList, (ListItem*)(vmacSta_p->EthStaCtl->EthStaDb+i));
    }

    vmacSta_p->EthStaCtl->eInitialized = TRUE;
    
    os_SemaphoreInit(&EthstaSH, 1);
    return(OS_SUCCESS);
}

static extStaDb_Status_e EthStaLocateAddr(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p,
                                    EthStaItem_t **Item_pp,
                                    UINT32 *Idx_p )
{
    UINT32 key;
    vmacApInfo_t *vmacSta_p = NULL;
    
    if (NULL == vmac_p || NULL == Addr_p || NULL == *Item_pp || NULL == Idx_p)
        return LOCATE_FAILURE;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    /*---------------------------------------------------------------------*/
    /* First, get the lower 32 bits of the MAC address to use for hashing. */
    /*---------------------------------------------------------------------*/
    memcpy(&key,
        ((IEEEtypes_Addr_t *)Addr_p + 2*sizeof(IEEEtypes_Addr_t)),
        4*sizeof(IEEEtypes_Addr_t));
    /*-----------------------------------------------------------*/
    /* Next, hash to get an index into the table that stores MAC */
    /* addresses and associated information.                     */
    /*-----------------------------------------------------------*/
    *Idx_p = Hash(key);

    /*-------------------------------------------------*/
    /* Now see if the address is already in the table. */
    /*-------------------------------------------------*/
    *Item_pp = vmacSta_p->EthStaCtl->EthStaDb_p[*Idx_p];
    if ( *Item_pp == NULL )
    {
        return(LOCATE_FAILURE);
    }
    else
    {
        if ( !memcmp((*Item_pp)->ethStaInfo.Addr,
            Addr_p,
            6*sizeof(IEEEtypes_Addr_t)) )
        {
            return(LOCATE_SUCCESS);
        }
        else
        {
            while ( (*Item_pp)->nxt_ht != NULL )
            {
                *Item_pp = (*Item_pp)->nxt_ht;

                if ( !memcmp((*Item_pp)->ethStaInfo.Addr,
                    Addr_p,
                    6*sizeof(IEEEtypes_Addr_t)) )
                {
                    return(LOCATE_SUCCESS);
                }
            }
        }
    }

    return(LOCATE_FAILURE);
}

extStaDb_Status_e ethStaDb_AddSta (vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p, extStaDb_StaInfo_t *StaInfo_p)
{
    extStaDb_Status_e result;
    EthStaItem_t *item_p = NULL;
    UINT32 idx;
    ListItem *tmp;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p || NULL == Addr_p || NULL == StaInfo_p)
        return NOT_INITIALIZED;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    if (!vmacSta_p->EthStaCtl->eInitialized)
    {
        return(NOT_INITIALIZED);
    }



    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(EthstaSH);

    /*------------------------------------------------------------------*/
    /* In the table, find a spot where the station that is to be added  */
    /* can be placed; if the station is already in the table, give back */
    /* the semaphore and return status.                                 */
    /*------------------------------------------------------------------*/
    if ((result = EthStaLocateAddr(vmacSta_p, Addr_p, &item_p, &idx)) !=
        LOCATE_FAILURE)
    {
        /* Update source path only */
        item_p->ethStaInfo.pStaInfo_t = StaInfo_p;
        os_SemaphorePut(EthstaSH);
        return(STATION_EXISTS_ERROR);
    }

    /*---------------------------------------------------------------*/
    /* Get a structure off of the free list, fill it out with the    */
    /* information about the new station, and put it in the location */
    /* found above.                                                  */
    /*---------------------------------------------------------------*/

    tmp = ListGetItem(&vmacSta_p->EthStaCtl->FreeEthStaList);
    if(tmp)
    {
        EthStaItem_t *search_p = vmacSta_p->EthStaCtl->EthStaDb_p[idx];
        item_p = (EthStaItem_t *)tmp;
        //there is one important element can not be wiped out, save it first

        memset(&item_p->ethStaInfo, 0, sizeof(eth_StaInfo_t));
        memcpy(&item_p->ethStaInfo.Addr, Addr_p,
            sizeof(IEEEtypes_MacAddr_t));
        item_p->ethStaInfo.pStaInfo_t = StaInfo_p;

        if(search_p)
        {   /*if hash table index idx already exist */
            while(search_p->nxt_ht)
            {
                search_p = search_p->nxt_ht;
            }
            search_p->nxt_ht = item_p;
            item_p->prv_ht = search_p;
            item_p->nxt_ht = NULL;
        }
        else
        {   /*put item to hash table idx */
            item_p->nxt_ht = item_p->prv_ht = NULL;
            vmacSta_p->EthStaCtl->EthStaDb_p[idx] = item_p;
        }

        ListPutItem(&vmacSta_p->EthStaCtl->EthStaList,tmp);
        item_p->ethStaInfo.TimeStamp = vmacSta_p->EthStaCtl->aging_time_in_minutes;

        /*-------------------------------------------------------*/
        /* Finished - give back the semaphore and return status. */
        /*-------------------------------------------------------*/
        os_SemaphorePut(EthstaSH);
        return (ADD_SUCCESS);
    }
    else
    {
        /*-------------------------------------------------------------*/
        /* There is no room in the table to add the station; give back */
        /* the semaphore and return status.                            */
        /*-------------------------------------------------------------*/
        /* Remove the least active station, and use the space for the new
        * one */
        os_SemaphorePut(EthstaSH);
        return (TABLE_FULL_ERROR);
    }
}


extStaDb_Status_e ethStaDb_DelSta (vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p, int option )
{
    extStaDb_Status_e result;
    EthStaItem_t *item_p = NULL;
    UINT32 idx;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p || NULL == Addr_p)
        return NOT_INITIALIZED;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    if (!vmacSta_p->EthStaCtl->eInitialized)
    {
        return(NOT_INITIALIZED);
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    if ( option )
        os_SemaphoreGet(EthstaSH);
    /*--------------------------------------------------------------*/
    /* In the table, find the station that is to be deleted; if not */
    /* found, give back the semaphore and return error status.      */
    /*--------------------------------------------------------------*/
    if ((result = EthStaLocateAddr(vmacSta_p, Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        if ( option )
            os_SemaphorePut(EthstaSH);
        return(result);
    }
    /*----------------------------------------*/
    /* Put the element back on the free list. */
    /*----------------------------------------*/
    {
        EthStaItem_t *search_p = vmacSta_p->EthStaCtl->EthStaDb_p[idx];
        if(search_p)
        {
            while (memcmp(&(search_p->ethStaInfo.Addr),
                item_p->ethStaInfo.Addr, 6*sizeof(IEEEtypes_Addr_t)))
            {
                search_p = search_p->nxt_ht;
            }
        }
        if(search_p && item_p)
        {
            if (search_p->prv_ht && search_p->nxt_ht)
            {   /*middle element */
                item_p->nxt_ht->prv_ht = item_p->prv_ht;
                item_p->prv_ht->nxt_ht = item_p->nxt_ht;
            }
            else
            {
                if(search_p->prv_ht)
                {   /*this is tail */
                    search_p->prv_ht->nxt_ht = NULL;
                }
                else if(search_p->nxt_ht)
                {   /*this is header */
                    search_p->nxt_ht->prv_ht = NULL;
                    vmacSta_p->EthStaCtl->EthStaDb_p[idx] =search_p->nxt_ht;
                }
                else
                {   /*one and only */
                    vmacSta_p->EthStaCtl->EthStaDb_p[idx] = NULL;
                }
            }
            item_p->nxt_ht = item_p->prv_ht = NULL;
        }
    }
    
    item_p->ethStaInfo.pStaInfo_t = NULL;
    memset(item_p->ethStaInfo.Addr, 0, sizeof(IEEEtypes_MacAddr_t));

    ListPutItem(&vmacSta_p->EthStaCtl->FreeEthStaList, ListRmvItem(&vmacSta_p->EthStaCtl->EthStaList,(ListItem *)item_p));
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
    if ( option )
        os_SemaphorePut(EthstaSH);

    return(DEL_SUCCESS);
}

eth_StaInfo_t  *ethStaDb_GetStaInfo(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p, int option)
{
    extStaDb_Status_e result;
    EthStaItem_t *item_p = NULL;
    UINT32 idx; 
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p || NULL == Addr_p)
        return NULL;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    if (!vmacSta_p->EthStaCtl->eInitialized)
    {
        return NULL;
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    if ( option )
        os_SemaphoreGet(EthstaSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /* if not found, give back the semaphore and return error status.   */
    /*------------------------------------------------------------------*/
    if ((result = EthStaLocateAddr(vmacSta_p, Addr_p, &item_p, &idx)) != LOCATE_SUCCESS)
    {
        if ( option  )
            os_SemaphorePut(EthstaSH);
        return NULL;
    }

    /*-------------------------------------------*/
    /* Fill out the requested state information. */
    /*-------------------------------------------*/
    item_p->ethStaInfo.TimeStamp = vmacSta_p->EthStaCtl->aging_time_in_minutes;
    /*-------------------------------------------------------*/
    /* Finished - give back the semaphore and return status. */
    /*-------------------------------------------------------*/
        if ( option )
            os_SemaphorePut(EthstaSH);

    /* For multiple BSS */
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
        if((item_p->ethStaInfo.pStaInfo_t) && memcmp(&vmac_p->macBssId, &((item_p->ethStaInfo.pStaInfo_t)->Bssid), sizeof(IEEEtypes_MacAddr_t)))
            return NULL;

    return &item_p->ethStaInfo;
}

void ethStaDb_RemoveAllStns(vmacApInfo_t *vmac_p)
{
    EthStaItem_t *Curr_p, *Item_p;
    eth_StaInfo_t *ethStaInfo_p;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p)
        return;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    Curr_p = (EthStaItem_t *)(vmacSta_p->EthStaCtl->EthStaList.head);
    while ( Curr_p != NULL )
    {
        Item_p = Curr_p;
        {
            /* Item can be aged, This involves removing the item
            * from the list and also, send message to the station
            */
            if ( (ethStaInfo_p = ethStaDb_GetStaInfo(vmac_p, &(Item_p->ethStaInfo.Addr), 0)) == NULL )
            {
                /* Station not known, do nothing */
                Curr_p = Curr_p->nxt;
                break;
            }
            ethStaDb_DelSta(vmac_p, &(Item_p->ethStaInfo.Addr), 0);
            Curr_p = (EthStaItem_t *)(vmacSta_p->EthStaCtl->EthStaList.head);
        }
    }
    
    return;
}

UINT16 ethStaDb_list(vmacApInfo_t *vmac_p)
{
    ListItem *search;
    EthStaItem_t *search1;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p)
        return 0;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    if (!vmacSta_p->EthStaCtl->eInitialized)
    {
        return (0);
    }
    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(EthstaSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/
    search = vmacSta_p->EthStaCtl->EthStaList.head;
    while (search)
    {
        search1 = (EthStaItem_t *)search;
        {
            struct extStaDb_StaInfo_t *pStaInfo_t = search1->ethStaInfo.pStaInfo_t;
            UINT8 *p = (UINT8 *)search1->ethStaInfo.Addr;

            if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA) {
                if ((search1->ethStaInfo.pStaInfo_t) && !memcmp(&vmac_p->macBssId, &((search1->ethStaInfo.pStaInfo_t)->Bssid), sizeof(IEEEtypes_MacAddr_t))) 
                {
                    printk("eth:%02x%02x%02x%02x%02x%02x  ", p[0], p[1], p[2], p[3], p[4], p[5]);
                    p = (UINT8 *)((search1->ethStaInfo.pStaInfo_t)->Addr);
                    if (pStaInfo_t)
                        printk("wl:%02x%02x%02x%02x%02x%02x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
                    else
                        printk("\n");
                }
            }
        }
        search = search->nxt;
    }

    os_SemaphorePut(EthstaSH);

    return (1);
}

extStaDb_Status_e ethStaDb_RemoveSta(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p)
{
    eth_StaInfo_t *pEthStaInfo;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p || NULL == Addr_p)
        return LOCATE_FAILURE;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    if ( (pEthStaInfo = ethStaDb_GetStaInfo(vmac_p, Addr_p, 0)) == NULL )
    {
        /* Station not known, do nothing */
        return LOCATE_FAILURE;
    }
    ethStaDb_DelSta(vmac_p, Addr_p, 0);

    return(DEL_SUCCESS);
}

extStaDb_Status_e ethStaDb_RemoveStaPerWlan(vmacApInfo_t *vmac_p, IEEEtypes_MacAddr_t *Addr_p)
{
    EthStaItem_t *Curr_p, *Item_p;
    eth_StaInfo_t *ethStaInfo_p;
    vmacApInfo_t *vmacSta_p = NULL;

    if (NULL == vmac_p || NULL == Addr_p)
        return LOCATE_FAILURE;
    vmacSta_p = vmac_p;
    if (vmac_p->VMacEntry.modeOfService != VMAC_MODE_CLNT_INFRA)
    {
        if(vmac_p->master)
            vmacSta_p= vmac_p->master;
    }

    os_SemaphoreGet(EthstaSH);
    
    Curr_p = (EthStaItem_t *)(vmacSta_p->EthStaCtl->EthStaList.head);
    while ( Curr_p != NULL )
    {
        Item_p = Curr_p;
        {
            /* Item can be aged, This involves removing the item
            * from the list and also, send message to the station
            */
            if ( (ethStaInfo_p = ethStaDb_GetStaInfo(vmac_p, &(Item_p->ethStaInfo.Addr), 0)) == NULL )
            {
                /* Station not known, do nothing */
                Curr_p = Curr_p->nxt;
                break;
            }
            if (Item_p->ethStaInfo.pStaInfo_t && 
                !memcmp(Addr_p, Item_p->ethStaInfo.pStaInfo_t->Addr, sizeof(IEEEtypes_MacAddr_t))) {
                ethStaDb_DelSta(vmac_p, &(Item_p->ethStaInfo.Addr), 0);
                Curr_p = (EthStaItem_t *)(vmacSta_p->EthStaCtl->EthStaList.head);
            }
            else
                Curr_p = Curr_p->nxt;
        }
    }

    os_SemaphorePut(EthstaSH);
    return(DEL_SUCCESS);
}
void ethStaDb_ProcessAgeTick(UINT8 *data)
{
    vmacApInfo_t *vmacSta_p = NULL;
    EthStaItem_t *Curr_p, *Item_p;
    eth_StaInfo_t *ethStaInfo_p;
    UINT32 count=0;

    WLDBG_INFO(DBG_LEVEL_10, "ethStaDb_ProcessAgeTick \n");

    if (NULL == data)
        return;
    vmacSta_p = (vmacApInfo_t *)data;
    Curr_p = (EthStaItem_t *)(vmacSta_p->EthStaCtl->EthStaList.head);
    while (Curr_p != NULL)
    {
        if(count++>EXT_STA_TABLE_SIZE)
            break;
        Item_p = Curr_p;
        if (--Item_p->ethStaInfo.TimeStamp)
        {
            Curr_p = Curr_p->nxt;
        }
        else
        {
            /* Item can be aged, This involves removing the item from the list  */
            if ((ethStaInfo_p = ethStaDb_GetStaInfo(vmacSta_p, &(Item_p->ethStaInfo.Addr), 0)) == NULL)
            {
                /* Station not known, do nothing */
                return ;
            }
            
            ethStaDb_DelSta(vmacSta_p,&(Item_p->ethStaInfo.Addr),0);

            WLDBG_EXIT(DBG_LEVEL_10);

            return;
        }
    }

    return;
}

extern int set_rptrSta_aging_time(vmacApInfo_t *vmac_p,int minutes)
{
    vmacApInfo_t *vmacSta_p;

    if (NULL == vmac_p)
        return 0;
    if(vmac_p->master)
        vmacSta_p= vmac_p->master;
    else
        vmacSta_p = vmac_p;
    
    vmacSta_p->EthStaCtl->aging_time_in_minutes = minutes * 60 / AGING_TIMER_VALUE_IN_SECONDS;
    return minutes;
}

void ieee80211_node_authorize(extStaDb_StaInfo_t *StaInfo_p,vmacApInfo_t *vmacSta_p)
{
/*
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
*/
    vmacApInfo_t *vmacSta;
    struct wlprivate *priv = NULL;

    if (NULL == StaInfo_p || NULL == vmacSta_p)
        return;

    priv = NETDEV_PRIV_P(struct wlprivate, vmacSta_p->dev);
    
    if(vmacSta_p->master)
        vmacSta = vmacSta_p->master;
    else
        vmacSta = vmacSta_p;

    WlLogPrint(MARVEL_DEBUG_WARNING, __func__,"sta:%02X:%02X:%02X:%02X:%02X:%02X AUTHORIZE SUCCESS!\n",\
            StaInfo_p->Addr[0],StaInfo_p->Addr[1],StaInfo_p->Addr[2],StaInfo_p->Addr[3],\
            StaInfo_p->Addr[4],StaInfo_p->Addr[5]);

    StaInfo_p->ni_flags |= IEEE80211_NODE_AUTH;
/*
    if (nt) {
        ni->ni_inact_reload = nt->nt_inact_run;
    }
    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;
*/

    if(vmacSta->StaCtl != NULL)
    {
            StaInfo_p->TimeStamp = vmacSta->StaCtl->aging_time_in_minutes;  /* AUTH_TIME; */
    }

    /*zhaoyang1 transplant from 717*/
    /*suzhaoyu add for customer online-traffic limit*/
    memset(&StaInfo_p->sta_stats, 0, sizeof(struct ieee80211_stastats));
    
    if(priv->vap.lowest_traffic_limit_switch)
    {
            extStaDb_online_traffic_timer_fn(&priv->vap,StaInfo_p);
    }
    /*suzhaoyu add end*/
    /*zhaoyang1 transplant end*/
    return;
}

void ieee80211_node_unauthorize(extStaDb_StaInfo_t *StaInfo_p)
{
/*
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
*/
    if (NULL == StaInfo_p)
        return;

    StaInfo_p->ni_flags &= ~IEEE80211_NODE_AUTH;
/*
    if (nt) {
        ni->ni_inact_reload = nt->nt_inact_auth;
    }
    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;
*/
    return;
}

int ieee80211_node_is_authorize(extStaDb_StaInfo_t *StaInfo_p)
{
    if (StaInfo_p == NULL)
    {
        return IEEE80211_NODE_AUTHORIZE_FAIL;
    }

    if ((StaInfo_p->ni_flags & IEEE80211_NODE_AUTH) != 0)
    {
        return IEEE80211_NODE_AUTHORIZE_SUCC;
    }
    else
    {
        return IEEE80211_NODE_AUTHORIZE_FAIL;
    }
}

/* add by zhanxuechao for online traffic process */
void extStaDb_online_traffic_timer_process_tick(UINT8 *data)
{
    union iwreq_data wreq;
    extStaDb_StaInfo_t *StaInfo_p = NULL;
    UINT32 check_interval;

    if (NULL == data)
        return;
    StaInfo_p = (extStaDb_StaInfo_t *)data;
    if(StaInfo_p->lowest_traffic_limit_switch==0)
    {
        return;
    }

    if (StaInfo_p->lowest_traffic_limit_timelength <= 0)
    {
        return;
    }

    if (StaInfo_p->lowest_traffic_limit_threshold <= 0)
    {
        return;
    }


    if(StaInfo_p->lowest_traffic_limit_timelength < 60)
            check_interval = StaInfo_p->lowest_traffic_limit_timelength;
    else
            check_interval = 60;

    if((StaInfo_p->sta_stats.ns_rx_bytes)-(StaInfo_p->sta_stats.ns_rx_bytes_l) == 0 && (StaInfo_p->sta_stats.ns_time_remain) >= (StaInfo_p->lowest_traffic_limit_timelength))
    {
        goto send_mac;
    }
    else if (((StaInfo_p->sta_stats.ns_rx_bytes)+(StaInfo_p->sta_stats.ns_tx_bytes)-(StaInfo_p->sta_stats.ns_sum_bytes)) < (StaInfo_p->lowest_traffic_limit_threshold))
    {
        if(StaInfo_p->sta_stats.ns_time_remain >= StaInfo_p->lowest_traffic_limit_timelength)
        {
            goto send_mac;
        }
        else
        {
            goto set_timer;
        }
    }
    else
    {
        StaInfo_p->sta_stats.ns_rx_bytes_l=StaInfo_p->sta_stats.ns_rx_bytes;
        StaInfo_p->sta_stats.ns_sum_bytes=StaInfo_p->sta_stats.ns_rx_bytes+StaInfo_p->sta_stats.ns_tx_bytes;
        StaInfo_p->sta_stats.ns_time_remain = 0;
        goto set_timer;
    }

send_mac:
    WlLogPrint(MARVEL_DEBUG_WARNING, __func__,"extStaDb_online_traffic_timer_process_tick send message.\n");
    memset(&wreq, 0, sizeof(wreq));
    memcpy(wreq.addr.sa_data, StaInfo_p->Addr, 6);
    wreq.addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(StaInfo_p->dev, IWEVTRAFFIC, &wreq, NULL);
    StaInfo_p->sta_stats.ns_rx_bytes_l=StaInfo_p->sta_stats.ns_rx_bytes;
    StaInfo_p->sta_stats.ns_sum_bytes=StaInfo_p->sta_stats.ns_rx_bytes+StaInfo_p->sta_stats.ns_tx_bytes;
    StaInfo_p->sta_stats.ns_time_remain = 0;
set_timer:
    WlLogPrint(MARVEL_DEBUG_DEBUG, __func__,"remain = %lu, interval = %lu, timelength = %lu\n",StaInfo_p->sta_stats.ns_time_remain,check_interval,StaInfo_p->lowest_traffic_limit_timelength);
    if(StaInfo_p->sta_stats.ns_time_remain + check_interval <= StaInfo_p->lowest_traffic_limit_timelength)
    {
            TimerFireIn(&StaInfo_p->online_traffic_timer, 1, &extStaDb_online_traffic_timer_process_tick, (unsigned char *)StaInfo_p, check_interval*10 );
        StaInfo_p->sta_stats.ns_time_remain += check_interval;
    }
    else
    {
            TimerFireIn(&StaInfo_p->online_traffic_timer, 1, &extStaDb_online_traffic_timer_process_tick, (unsigned char *)StaInfo_p, (StaInfo_p->lowest_traffic_limit_timelength -StaInfo_p->sta_stats.ns_time_remain)*10 );
        StaInfo_p->sta_stats.ns_time_remain += (StaInfo_p->lowest_traffic_limit_timelength -StaInfo_p->sta_stats.ns_time_remain);
    }
}

void extStaDb_online_traffic_timer_fn(struct ieee80211vap *vap, extStaDb_StaInfo_t *StaInfo_p)
{
    UINT32 timer_length = 0;

    if (NULL == vap || NULL == StaInfo_p)
    {
        return;
    }
    
    if (vap->lowest_traffic_limit_switch == 0)
    {
        return;
    }

    if (vap->lowest_traffic_limit_timelength <= 0)
    {
        return;
    }

    if (vap->lowest_traffic_limit_threshold <= 0)
    {
        return;
    }

    StaInfo_p->lowest_traffic_limit_switch = vap->lowest_traffic_limit_switch;
    StaInfo_p->lowest_traffic_limit_timelength = vap->lowest_traffic_limit_timelength;
    StaInfo_p->lowest_traffic_limit_threshold = vap->lowest_traffic_limit_threshold;

    /** Start aging timer */
    TimerInit(&StaInfo_p->online_traffic_timer);

    StaInfo_p->sta_stats.ns_rx_bytes_l= StaInfo_p->sta_stats.ns_rx_bytes;
    StaInfo_p->sta_stats.ns_sum_bytes= StaInfo_p->sta_stats.ns_rx_bytes + StaInfo_p->sta_stats.ns_tx_bytes;

    if (StaInfo_p->lowest_traffic_limit_timelength < 60)
    {
        timer_length = StaInfo_p->lowest_traffic_limit_timelength;      
        StaInfo_p->sta_stats.ns_time_remain =StaInfo_p->lowest_traffic_limit_timelength;
    }
    else
    {
        timer_length = 60;
        StaInfo_p->sta_stats.ns_time_remain += 60;
    }

    TimerFireIn(&StaInfo_p->online_traffic_timer, 1, &extStaDb_online_traffic_timer_process_tick, (unsigned char *)StaInfo_p, timer_length*10 );
}

UINT16 extStaDb_online_traffic_set_timer(vmacApInfo_t *vmac_p)
{
    int j;
    ListItem *search;
    ExtStaInfoItem_t *search1;
    vmacApInfo_t *vmacSta_p;
    struct wlprivate *priv = NULL;

    if (NULL == vmac_p)
    {
        return 0;
    }

    priv = NETDEV_PRIV_P(struct wlprivate, vmac_p->dev);

    if(vmac_p->master)
    {
        vmacSta_p= vmac_p->master;
    }
    else
    {
        vmacSta_p = vmac_p;
    }

    if (!vmacSta_p->StaCtl->Initialized)
    {
        return 0;
    }

    if(priv->vap.lowest_traffic_limit_switch == 0)
    {
        return 0;
    }

    /*-----------------------------------------------------------------*/
    /* Get the semaphore to gain access to the table; this may involve */
    /* a wait if the semaphore is currently held by another task.      */
    /*-----------------------------------------------------------------*/
    os_SemaphoreGet(staSH);

    /*------------------------------------------------------------------*/
    /* In the table, find the station for which the state is requested; */
    /*------------------------------------------------------------------*/

    j = 0;

    search = vmacSta_p->StaCtl->StaList.head;
    while (search)
    {
        search1 = (ExtStaInfoItem_t *)search;
        if ((search1->StaInfo.AP == FALSE) && (search1->StaInfo.State == ASSOCIATED)
         && (!memcmp(&vmac_p->macBssId, &search1->StaInfo.Bssid,sizeof(IEEEtypes_MacAddr_t))))
        {
            extStaDb_online_traffic_timer_fn(&priv->vap, &search1->StaInfo);
        }
        search = search->nxt;
    }

    os_SemaphorePut(staSH);

    return 1;
}

