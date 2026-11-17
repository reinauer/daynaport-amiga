/*
 * SCSI DaynaPORT Device (scsidayna.device) Copyright (C) 2024-2026 RobSmithDev 
 *
 *
 * Originally based on the MNT ZZ9000 Network Driver Copyright (C) 2016-2023, Lukas F. Hartmann <lukas@mntre.com>
 *          which was Based on code copyright (C) 2018 Henryk Richter <henryk.richter@gmx.net>
 * Released under GPLv3+ with permission.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * GNU General Public License v3.0 or later
 *
 * https://spdx.org/licenses/GPL-3.0-or-later.html
 */

#ifndef _INC_DEVICE_H
#define _INC_DEVICE_H

/* includes */
#include "compiler.h"
#include <dos/dos.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include "debug.h"
#include "sana2.h"

/* reassign Library bases from global definitions to own struct */
#define SysBase       db->db_SysBase
#define DOSBase       db->db_DOSBase
#define UtilityBase   db->db_UtilityBase

/* multicast groups kept (db_mcastPending has a bit per entry) */
#define MCAST_MAX 16
/* distinct packet types remembered from CMD_READ */
#define READ_TYPES_MAX 8

struct devbase {
	struct Library db_Lib;
	BPTR db_SegList;            /* from Device Init */

	struct Library *db_SysBase; /* Exec Base */
	struct Library *db_DOSBase;
	struct Library *db_UtilityBase;
	struct Sana2DeviceStats db_DevStats;
	
	BPTR db_debugConsole;  // I couldnt get any form of S2_SANA2HOOK working	
	BOOL db_quit;
	BOOL db_timerUsed ;
		
	volatile USHORT db_online;
	volatile USHORT db_currentWifiState;   // the *actual* online state
	USHORT db_amigaNetMode;
	USHORT db_maxPacketsSize;		// Maximum size of packet data (multiple packets)
	USHORT db_maxPackets;			// Maximum number of supported packets per call
	
    // SCSI device (in the main task)
	void* db_scsiSettings;    // A pointer to a ScsiDaynaSettings struct  
	struct List db_ReadList;
	struct SignalSemaphore db_ReadListSem;
	struct List db_WriteList;
	struct SignalSemaphore db_WriteListSem;
	struct List db_EventList;
	struct SignalSemaphore db_EventListSem;   
	struct List db_ReadOrphanList;
	struct SignalSemaphore db_ReadOrphanListSem;
	struct Process* db_Proc;
	struct SignalSemaphore db_ProcSem;
	/* Held across DevOpen and DevClose: both wait (SCSI I/O, process start
	 * and stop), which breaks exec's Forbid and lets another opener in. */
	struct SignalSemaphore db_OpenSem;
	/* Multicast groups (S2_ADDMULTICASTADDRESS). The DaynaPORT takes them over
	 * SCSI (command 0x0D), which only the packet server process may do: the
	 * caller's BeginIO records the address here and signals it. */
	UBYTE db_mcast[MCAST_MAX][6];
	UWORD db_mcastRefs[MCAST_MAX];         /* adds minus deletes, per address */
	volatile USHORT db_mcastCount;          /* addresses in the table */
	volatile UWORD db_mcastPending;         /* bit n: entry n not given to the adapter yet */
	struct SignalSemaphore db_mcastSem;
	/* Packet types the stack has posted CMD_READs for (under db_ReadListSem).
	 * A packet of such a type with no read waiting is worth a short wait for
	 * the stack to post one; anything else is dropped at once. */
	UWORD db_readTypes[READ_TYPES_MAX];
	volatile USHORT db_readTypeCount;
	/* Set by the packet task while it waits for a read buffer: CMD_READ and
	 * S2_READORPHAN then signal db_readSigMask to wake it */
	volatile UBYTE db_readStarved;
	ULONG db_readSigMask;
};

#ifndef DEVBASETYPE
#define DEVBASETYPE struct devbase
#endif
#ifndef DEVBASEP
#define DEVBASEP DEVBASETYPE *db
#endif

/* PROTOS */

ASM LONG LibNull( void );

ASM SAVEDS struct Device *DevInit(ASMR(d0) DEVBASEP                  ASMREG(d0), 
                                  ASMR(a0) BPTR seglist              ASMREG(a0), 
				  ASMR(a6) struct Library *_SysBase  ASMREG(a6) );

ASM SAVEDS LONG DevOpen( ASMR(a1) struct IOSana2Req *ios2            ASMREG(a1), 
                         ASMR(d0) ULONG unit                         ASMREG(d0), 
                         ASMR(d1) ULONG flags                        ASMREG(d1),
                         ASMR(a6) DEVBASEP                           ASMREG(a6) );

ASM SAVEDS BPTR DevClose(   ASMR(a1) struct IORequest *ios2         ASMREG(a1),
                            ASMR(a6) DEVBASEP                        ASMREG(a6) );

ASM SAVEDS BPTR DevExpunge( ASMR(a6) DEVBASEP                        ASMREG(a6) );

ASM SAVEDS VOID DevBeginIO( ASMR(a1) struct IOSana2Req *ios2         ASMREG(a1),
                            ASMR(a6) DEVBASEP                        ASMREG(a6) );

ASM SAVEDS LONG DevAbortIO( ASMR(a1) struct IORequest *ios2         ASMREG(a1),
                            ASMR(a6) DEVBASEP                        ASMREG(a6) );

void DevTermIO( DEVBASETYPE*, struct IORequest * );

/* private functions */
#ifdef DEVICE_MAIN

#endif /* DEVICE_MAIN */

#define HW_ADDRFIELDSIZE          6
#define HW_ETH_HDR_SIZE          14       /* ethernet header: dst, src, type */

typedef BOOL (*BMFunc)(ASMR(a0) void* a ASMREG(a0),
                       ASMR(a1) void* b ASMREG(a1),
                       ASMR(d0) long c ASMREG(d0));

/*
  The 16 bit variants may assume the driver-side buffer is word aligned.
  Packets in an AmigaNET batch can start at odd addresses, so both
  variants are kept and BM_COPYFROM()/BM_COPYTO() pick the right one.
*/
typedef struct BufferManagement
{
  struct MinNode   bm_Node;
  BMFunc           bm_CopyFromBuffer;    /* S2_CopyFromBuff, always set */
  BMFunc           bm_CopyToBuffer;      /* S2_CopyToBuff, always set */
  BMFunc           bm_CopyFromBuffer16;  /* S2_CopyFromBuff16 or NULL */
  BMFunc           bm_CopyToBuffer16;    /* S2_CopyToBuff16 or NULL */
} BufferManagement;

#define BM_PICK(_fn16_, _fn_, _buf_) \
	(((_fn16_) && !((ULONG)(_buf_) & 1)) ? (_fn16_) : (_fn_))
/* copy from the stack's buffer into driver memory at _to_ */
#define BM_COPYFROM(_bm_, _to_, _from_, _n_) \
	(*BM_PICK((_bm_)->bm_CopyFromBuffer16, (_bm_)->bm_CopyFromBuffer, (_to_)))((_to_), (_from_), (_n_))
/* copy from driver memory at _from_ into the stack's buffer */
#define BM_COPYTO(_bm_, _to_, _from_, _n_) \
	(*BM_PICK((_bm_)->bm_CopyToBuffer16, (_bm_)->bm_CopyToBuffer, (_from_)))((_to_), (_from_), (_n_))

#endif /* _INC_DEVICE_H */
