/* da_gc_pe.c
 *
 * Demo Application for the Packet Engine Global Control API
 * Linux kernel-space and user-space.
 */

/*****************************************************************************
* Copyright (c) 2012-2022 by Rambus, Inc. and/or its subsidiaries.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

// Demo Application Packet Engine Module API
#include "da_gc_pe.h"


/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

// Default configuration
#include "c_da_gc_pe.h"         // configuration switches

// Driver Framework Basic Definitions API
#include "basic_defs.h"         // uint8_t, uint32_t, bool

// Global Control Packet I/O API
#include "api_global_eip97.h"  // GlobalControl97_Init/Capabilities_Get/UnInit

// Global Control Packet I/O Status API
#include "api_global_status_eip97.h"

// Driver Framework C Library API
#include "clib.h"               // memcpy, ZEROINIT

#include "device_types.h"       // Device_Handle_t
#include "device_mgmt.h"        // Device_find
#include "log.h"                // Log API


/*----------------------------------------------------------------------------
 * Definitions and macros
 */


/*----------------------------------------------------------------------------
 * Local variables
 */

static const uint32_t Global_PRNG_Key[8] =
{
    0x48c24cfd, 0x6c07f742, 0xaee75681, 0x0f27c239,
    0x79947198, 0xe2991275, 0x21ac3c7c, 0xd008c4b4
};

/*----------------------------------------------------------------------------
 * BoolToString()
 *
 * Convert boolean value to string.
 */
static char *
BoolToString(
        const bool b)
{
    if (b)
        return "true";
    else
        return "false";
}


/*----------------------------------------------------------------------------
 * Global_StatusReport()
 *
 * Obtain all available global status information from the EIP-97 driver
 * and report it.
 */
void
Global_StatusReport(void)
{
    GlobalControl97_Error_t rc;
    unsigned int i;
    unsigned int NofPE;

    LOG_INFO("\n\t\t Global_StatusReport \n");

    LOG_CRIT("Global Status of the EIP-97\n");

    GlobalControl97_Interfaces_Get(&NofPE, NULL, NULL, NULL);
    for (i=0; i < NofPE; i++)
    {
        LOG_CRIT("Packet Engine %d Status\n", i);
        {
            GlobalControl97_DFE_Status_t DFE_Status;

            rc = GlobalControl97_DFE_Status_Get(i, &DFE_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("DFE Status: CD FIFO Words: %d, CDR ID: %d, DMA size: %d\n"
                     "AtDMA busy: %s, DataDMA busy: %s, DMA err: %s\n",
                     DFE_Status.CDFifoWord32Count,
                     DFE_Status.CDR_ID,
                     DFE_Status.DMASize,
                     BoolToString(DFE_Status.fAtDMABusy),
                     BoolToString(DFE_Status.fDataDMABusy),
                     BoolToString(DFE_Status.fDMAError));
        }

        {
            GlobalControl97_DSE_Status_t DSE_Status;

            rc = GlobalControl97_DSE_Status_Get(i, &DSE_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("DSE Status: RD FIFO Words: %d, RDR ID: %d, DMA size: %d\n"
                     "Data flush  busy: %s, DataDMA busy: %s, DMA err: %s\n",
                     DSE_Status.RDFifoWord32Count,
                     DSE_Status.RDR_ID,
                     DSE_Status.DMASize,
                     BoolToString(DSE_Status.fDataFlushBusy),
                     BoolToString(DSE_Status.fDataDMABusy),
                     BoolToString(DSE_Status.fDMAError));
        }

        {
            GlobalControl97_Token_Status_t Token_Status;
            rc = GlobalControl97_Token_Status_Get(i, &Token_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("Token Status: Active: %d, loc available: %s\n"
                     "res available: %s, read active: %s, ccache active: %s\n"
                     "cntx fetch: %s, res cntx: %s\n"
                     "processing held: %s, busy: %s\n",
                     Token_Status.ActiveTokenCount,
                     BoolToString(Token_Status.fTokenLocationAvailable),
                     BoolToString(Token_Status.fResultTokenAvailable),
                     BoolToString(Token_Status.fTokenReadActive),
                     BoolToString(Token_Status.fContextCacheActive),
                     BoolToString(Token_Status.fContextFetch),
                     BoolToString(Token_Status.fResultContext),
                     BoolToString(Token_Status.fProcessingHeld),
                     BoolToString(Token_Status.fBusy));
        }

        {
            GlobalControl97_Context_Status_t Context_Status;

            rc = GlobalControl97_Context_Status_Get(i, &Context_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("Context Status: Err mask: %04x, Available: %d\n"
                     "Active cntx: %s, next cntx: %s, result cntx: %s"
                     " Err recov: %s\n",
                     Context_Status.Error,
                     Context_Status.AvailableTokenCount,
                     BoolToString(Context_Status.fActiveContext),
                     BoolToString(Context_Status.fNextContext),
                     BoolToString(Context_Status.fResultContext),
                     BoolToString(Context_Status.fErrorRecovery));
        }

        {
            GlobalControl97_Interrupt_Status_t Interrupt_Status;

            rc = GlobalControl97_Interrupt_Status_Get(i, &Interrupt_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("Interrupt Status: input DMA err: %s, output DMA err %s \n"
                     "pkt proc err: %s, pkt timeout: %s, f a t a l err: %s, "
                     "PE int out: %s\n"
                     "inp DMA enable: %s, outp DMA enable %s, "
                     "pkt proc enable: %s\n"
                     "pkt timeout enable: %s, f a t a l enable: %s,"
                     "PE int out enable: %s\n",
                     BoolToString(Interrupt_Status.fInputDMAError),
                     BoolToString(Interrupt_Status.fOutputDMAError),
                     BoolToString(Interrupt_Status.fPacketProcessingError),
                     BoolToString(Interrupt_Status.fPacketTimeout),
                     BoolToString(Interrupt_Status.fFatalError),
                     BoolToString(Interrupt_Status.fPeInterruptOut),
                     BoolToString(Interrupt_Status.fInputDMAErrorEnabled),
                     BoolToString(Interrupt_Status.fOutputDMAErrorEnabled),
                     BoolToString(Interrupt_Status.fPacketProcessingEnabled),
                     BoolToString(Interrupt_Status.fPacketTimeoutEnabled),
                     BoolToString(Interrupt_Status.fFatalErrorEnabled),
                     BoolToString(Interrupt_Status.fPeInterruptOutEnabled));
        }
        {
            GlobalControl97_Output_Transfer_Status_t OutXfer_Status;

            rc = GlobalControl97_OutXfer_Status_Get(i, &OutXfer_Status);

            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                continue;
            }

            LOG_CRIT("Output Transfer Status: availabe: %d, "
                     "min: %d, max: %d, size mask: %d\n",
                     OutXfer_Status.AvailableWord32Count,
                     OutXfer_Status.MinTransferWordCount,
                     OutXfer_Status.MaxTransferWordCount,
                     OutXfer_Status.TransferSizeMask);
        }
        {
            GlobalControl97_PRNG_Status_t PRNG_Status;

            rc = GlobalControl97_PRNG_Status_Get(i, &PRNG_Status);

            if (rc == GLOBAL_CONTROL_ERROR_NOT_IMPLEMENTED)
            {
                LOG_CRIT("No PRNG present in EIP-96\n");
            }
            else if (rc == GLOBAL_CONTROL_NO_ERROR)
            {
                    LOG_CRIT("PRNG Status: busy: %s, res ready: %s\n",
                             BoolToString(PRNG_Status.fBusy),
                             BoolToString(PRNG_Status.fResultReady));
            }
        }
    }
#ifdef DA_GC_DBG_STATISTICS
    {
        GlobalControl97_Debug_Statistics_t Debug_Statistics;

        rc = GlobalControl97_Debug_Statistics_Get(&Debug_Statistics);

        if (rc != GLOBAL_CONTROL_NO_ERROR)
        {
           return;
        }

        LOG_CRIT("\nInterface DBG Statistics:\n");
        for (i=0; i<16; i++)
        {
            if(Debug_Statistics.Ifc_Packets_In[i] != 0 ||
               Debug_Statistics.Ifc_Packets_Out[i] != 0)
            {
                LOG_CRIT("\t\tInterface #%2u Input pkts=%10u Output pkts=%10u\n",
                         i,
                         (unsigned int)Debug_Statistics.Ifc_Packets_In[i],
                         (unsigned int)Debug_Statistics.Ifc_Packets_Out[i]);
            }
        }
        LOG_CRIT("\nPipe DBG Statistics:\n");
        for (i=0; i < NofPE; i++)
        {
            LOG_CRIT("\t\tPipe #%2u Total=%10u Data=%10u Cur=%3u Max=%3u\n",
                     i,
                     (unsigned int)Debug_Statistics.Pipe_Total_Packets[i],
                     (unsigned int)Debug_Statistics.Pipe_Data_Count[i],
                     (unsigned int)Debug_Statistics.Pipe_Current_Packets[i],
                     (unsigned int)Debug_Statistics.Pipe_Max_Packets[i]);
        }
    }
#endif
}


/*---------------------------------------------------------------------------
 * Global_Init()
 */
bool
Global_Init(void)
{
    GlobalControl97_Error_t rc = GLOBAL_CONTROL_NO_ERROR;
    unsigned int i;
    unsigned int NofPE;

    LOG_INFO("DA_GC: Global_Init \n");

    rc = GlobalControl97_Init(false);

    if (rc != GLOBAL_CONTROL_NO_ERROR)
    {
        LOG_CRIT("DA_GC: Global_Init, packet I/O initialization failed\n");
        return false;
    }

    // Enable the rings for the packet engines.
    // This version enables all configured rings on Packet Engine #0 only.
    {
        GlobalControl97_Ring_PE_Map_t RingPEMap;
        unsigned int NofRings, NofLAInterfaces, NofInlineInterfaces;

        ZEROINIT(RingPEMap);

        GlobalControl97_Interfaces_Get(&NofPE,
                                       &NofRings,
                                       &NofLAInterfaces,
                                       &NofInlineInterfaces);

        RingPEMap.RingPE_Mask = (1<< (NofRings + NofLAInterfaces + NofInlineInterfaces))-1;
        /* Enable the number of configured rings */
        RingPEMap.RingPrio_Mask = 0x00; /* All rings at standard priority */

        for (i=0; i < NofPE; i++)
        {
            rc = GlobalControl97_Configure(i, &RingPEMap);
            if (rc != GLOBAL_CONTROL_NO_ERROR)
            {
                LOG_CRIT("DA_GC: Ring configuration failed for PE %d\n", i);
                GlobalControl97_UnInit();
                return rc;
            }
        }
    }

    {
        GlobalControl97_PRNG_Status_t PRNG_Status;
        // Check whether we have a PRNG.
        rc = GlobalControl97_PRNG_Status_Get(0, &PRNG_Status);

        if (rc != GLOBAL_CONTROL_NO_ERROR && rc != GLOBAL_CONTROL_ERROR_NOT_IMPLEMENTED)
        {
                return false;
        }

        if (rc != GLOBAL_CONTROL_ERROR_NOT_IMPLEMENTED)
        {
            for (i=0; i < NofPE; i++)
            {
                GlobalControl97_PRNG_Reseed_t PRNG_Reseed;

                PRNG_Reseed.SeedLo = Global_PRNG_Key[0];
                PRNG_Reseed.SeedHi = Global_PRNG_Key[1];
                PRNG_Reseed.Key0Lo = Global_PRNG_Key[2];
                PRNG_Reseed.Key0Hi = Global_PRNG_Key[3];
                PRNG_Reseed.Key1Lo = Global_PRNG_Key[4];
                PRNG_Reseed.Key1Hi = Global_PRNG_Key[5];
                PRNG_Reseed.LFSRLo = Global_PRNG_Key[6];
                PRNG_Reseed.LFSRHi = Global_PRNG_Key[7];

                rc = GlobalControl97_PRNG_Reseed(i, &PRNG_Reseed);

                if (rc != GLOBAL_CONTROL_NO_ERROR)
                {
                    LOG_CRIT("Could not ressed PRNG of PE#%d\n",i);
                    GlobalControl97_UnInit();
                    return rc;
                }
            }
        }
    }

    {
        GlobalControl97_Capabilities_t Capabilities;

        rc = GlobalControl97_Capabilities_Get(&Capabilities);
        if ( rc != GLOBAL_CONTROL_NO_ERROR)
        {
            LOG_CRIT("GlobalControl97_Capabilities_Get returned error\n");
        }
        else
        {
            LOG_CRIT("Global EIP-97 capabilities: %s\n",
                     Capabilities.szTextDescription);
        }
    }

    Global_StatusReport();

    return true;
}


/*----------------------------------------------------------------------------
 * Global_UnInit()
 *
 */
void
Global_UnInit(void)
{
    LOG_INFO("DA_GC: Global_UnInit \n");

    Global_StatusReport();

    // Uninitialize Packet I/O Global Control
    GlobalControl97_UnInit();
}


/* end of file da_gc_pe.c */
