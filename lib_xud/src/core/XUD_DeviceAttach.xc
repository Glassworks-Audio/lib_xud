// Copyright 2011-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if !defined(XUD_BYPASS_RESET)
#include <xs1.h>
#include <platform.h>
#include "xud.h"
#include "XUD_USB_Defines.h"
#include "XUD_TimingDefines.h"
#include "XUD_HAL.h"

extern in  port flag0_port;
extern in  port flag1_port;
extern in  port flag2_port;
extern out buffered port:32 p_usb_txd;

#define TUCHEND_DELAY_us   (1500) // 1.5ms
#define TUCHEND_DELAY      (TUCHEND_DELAY_us * PLATFORM_REFERENCE_MHZ)

#ifndef INVALID_DELAY_us
#define INVALID_DELAY_us   (2500) // 2.5ms
#endif

#define INVALID_DELAY      (INVALID_DELAY_us * PLATFORM_REFERENCE_MHZ)

extern int resetCount;

/* What the last failed high-speed handshake saw, for the report XUD_Main
 * sends up c_usb_ctl. Written where the handshake gives up.
 *
 * g_xudHsChirp: bit 0 the host answered our chirp K with a chirp K of its
 *   own; bits 2-3 the flag0 / flag1 line flags were seen high while WE were
 *   driving chirp K; bits 4-7 the K-J pairs seen after the host's K, of the
 *   three that complete the handshake. 0 with bit 0 clear: the hub never
 *   chirped back at all.
 * g_xudHsFlags: bits 0-1 flag0 / flag1 just before entering chirp mode, i.e.
 *   what the reset decision rested on (SE0 reads 00); bits 2-3 the same just
 *   after entering chirp mode, before driving.
 * g_xudHsHeld: from the start of our chirp K to the end of the host's SE0,
 *   in 100 us units, capped at 255. A reset that ends within our own chirp
 *   is a chirp that came too late.
 * g_xudHsWait: bits 0-7 samples with flag0 high while we waited for the
 *   host's chirp, bits 8-15 the same for flag1 (one sample every 2 us, each
 *   capped at 255); bits 16-23 how long after XUD_Main saw the reset our
 *   chirp began, in 100 us units. The spec wants the chirp finished within
 *   7 ms of the reset starting. No samples while the reset ran on for
 *   milliseconds means the hub never chirped back; plenty of them and no
 *   pairs means it did and the detection missed it.
 *
 * 2026-09-03, on a unit whose high-speed front end had died: "host chirp NOT
 * seen, 0 pairs, flags 00 / 00, no wait samples, reset ran 8.5 ms past our
 * chirp" -- a clean reset with nothing at all on the wire. */
unsigned g_xudHsChirp = 0;
unsigned g_xudHsFlags = 0;
unsigned g_xudHsHeld = 0;
unsigned g_xudHsWait = 0;

/* When XUD_Main decided the bus was in reset, on the reference timer */
unsigned g_xudResetTime = 0;

/* Assumptions:
 * - In full speed mode
 * - No flags sticky
 * - Flag 0 port inverted
 */
int XUD_DeviceAttachHS(XUD_PwrConfig pwrConfig)
{
   unsigned tmp;
   timer t;
   int start_time;
   int detecting_k = 1;
   int tx;
   unsigned int chirpCount = 0;

   /* Line sampling while we wait, see g_xudHsWait */
   unsigned tChirpStart, tSample;
   unsigned n0 = 0, n1 = 0;
   unsigned pre0, pre1, post0, post1;

   clearbuf(p_usb_txd);

   /* The flags the reset decision was made on, still in full-speed mode */
   flag0_port :> pre0;
   flag1_port :> pre1;

   /* On detecting the SE0 move into chirp mode */
   XUD_HAL_EnterMode_PeripheralChirp();
   t :> tChirpStart;
   flag0_port :> post0;
   flag1_port :> post1;

   /* output k-chirp for required time */
   unsigned selfK = 0, selfJ = 0;
#if defined(XUD_SIM_RTL) || (XUD_SIM_XSIM)
   for (int i = 0; i < 800; i++)
#else
   for (int i = 0; i < 16000; i++)    // 16000 words @ 480 MBit = 1.066 ms
#endif
    {
        p_usb_txd <: 0;

        /* Glance at our own chirp on the line, every 256 words (~17 us):
         * two port reads, well inside the 32-bit port buffer's slack */
        if((i & 0xFF) == 0x80)
        {
            unsigned a, b;
            flag0_port :> a;
            flag1_port :> b;
            selfK |= a;
            selfJ |= b;
        }
    }

   // J, K, SE0 on flag ports 0, 1, 2 respectively (on XS2)
   // XS3 has raw linestate on flag port 0 and 1
   // Wait for fs chirp k (i.e. HS chirp j)
#if defined(__XS2A__)
    flag1_port when pinseq(0) :> tmp; // Wait for out k to go
#endif

    t :> start_time;
    tSample = start_time;
    while(1)
    {
        select
        {
            /* One timer serves both the 2 us line sampling and the give-up
             * deadline; the deadline is checked on each tick, so it can land
             * at most one sample late. */
            case t when timerafter(tSample) :> void:

                if((int)(tSample - (start_time + INVALID_DELAY)) < 0)
                {
                    unsigned a, b;
                    tSample += 2 * PLATFORM_REFERENCE_MHZ;
                    flag0_port :> a;
                    flag1_port :> b;
                    if(a) n0++;
                    if(b) n1++;
                    break;
                }

                /* Go into full speed mode: XcvrSelect and Term Select (and suspend) high */
                XUD_HAL_EnterMode_PeripheralFullSpeed();

                /* Wait for end of SE0 */
                while(1)
                {
                    /* TODO Use a timer to save some juice...*/
#if !defined(__XS2A__)
                    unsigned dp, dm;
                    flag0_port :> dm;
                    flag1_port :> dp;

                    if(dp || dm)
                    {
                        /* SE0 gone, return 0 to indicate FULL SPEED */
                        unsigned now, held, late;
                        t :> now;
                        held = (now - tChirpStart) / (100 * PLATFORM_REFERENCE_MHZ);
                        late = (tChirpStart - g_xudResetTime) / (100 * PLATFORM_REFERENCE_MHZ);
                        g_xudHsChirp = ((chirpCount > 0) || !detecting_k) | ((selfK ? 1 : 0) << 2)
                                     | ((selfJ ? 1 : 0) << 3) | (chirpCount << 4);
                        g_xudHsFlags = (pre0 & 1) | ((pre1 & 1) << 1) | ((post0 & 1) << 2) | ((post1 & 1) << 3);
                        g_xudHsHeld = (held > 255) ? 255 : held;
                        g_xudHsWait = (n0 > 255 ? 255 : n0) | ((n1 > 255 ? 255 : n1) << 8)
                                    | ((late > 255 ? 255 : late) << 16);
                        return 0;
                    }
#else
                    flag2_port :> tmp;

                    if(!tmp)
                    {
                        /* SE0 gone, return 0 to indicate FULL SPEED */
                        return 0;
                    }
#endif
                    if(pwrConfig == XUD_PWR_SELF)
                    {
                        if(!XUD_HAL_GetVBusState())
                        {
                            XUD_HAL_EnterMode_TristateDrivers();
                            return -1;             // VBUS gone, handshake fails completely.
                        }
                    }
                }
                break;

#if !defined(__XS2A__)
// Note, J and K definitions are reversed in XS3A
#define j_port flag1_port
#define k_port flag0_port
#else
#define k_port flag1_port
#define j_port flag0_port
#endif
            case detecting_k => k_port when pinseq(1):> void @ tx:       // K Chirp
                k_port @ tx + T_FILT_ticks :> tmp;
                if (tmp)
                {
                    detecting_k = 0;
                }
                break;

             case !detecting_k => j_port when pinseq(1) :> void @ tx:    // J Chirp
                j_port @ tx + T_FILT_ticks :> tmp;
                if (tmp == 1)
                {
                    chirpCount++;                                            // Seen an extra K-J pair
                    detecting_k = 1;

                    if (chirpCount == 3)
                    {
                        /* Three pairs of KJ received. Enter high-speed mode */
                        XUD_HAL_EnterMode_PeripheralHighSpeed();

                        // Wait for SE0 (TODO consume other chirps?)
#if !defined(__XS2A__)
                        // TODO ideally dont use a polling loop here
                        while (XUD_HAL_GetLineState() != XUD_LINESTATE_SE0);
#else
                        flag2_port when pinseq(1) :> tmp;
#endif

                        /* Return 1 to indicate successful HS handshake*/
                        return 1;

                    }
                }
                break;
        }
    }
    // Unreachable
    return -1;
}
#endif
