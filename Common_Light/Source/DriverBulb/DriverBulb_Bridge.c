/****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2016. All rights reserved
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/* SDK includes */
#include <jendefs.h>
#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"

/* Device includes */
#include "DriverBulb.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_DRIVER
#define TRACE_DRIVER FALSE
#else
#define TRACE_DRIVER TRUE
#endif

#define ADC_FULL_SCALE   1023

#ifndef BULBS_COUNT
#define BULBS_COUNT 1u
#error BULBS_COUNT
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t bBulbOn[BULBS_COUNT] = {0};
#ifdef CCT
PRIVATE	uint32 au32Mireds2RGB[17][4] =
{
/* Mireds,   Red, Green,  Blue */
	{   0,    50,   117,   255},
	{  63,    88,   153,   255},
	{ 127,   158,   208,   255},
	{ 191,   253,   255,   221},
	{ 255,   255,   202,   124},
	{ 319,   255,   163,    70},
	{ 383,   255,   135,    41},
	{ 447,   255,   112,    24},
	{ 511,   255,    95,    14},
	{ 575,   255,    80,     9},
	{ 639,   255,    70,     5},
	{ 703,   255,    60,     3},
	{ 767,   255,    52,     2},
	{ 831,   255,    45,     1},
	{ 895,   255,    40,     1},
	{ 959,   255,    35,     0},
	{1023,   255,    32,     0}
};
#endif
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void DriverBulb_vInit(uint8 u8index)
{
	static bool_t bFirstCalled_init[BULBS_COUNT] = {0};
	static bool_t bFirstCalled[BULBS_COUNT] = {0};

	if (bFirstCalled_init[u8index] == FALSE)
	{
		bFirstCalled[u8index] = TRUE;
		bFirstCalled_init[u8index] = TRUE;
	}

    DBG_vPrintf(TRUE, "\n#%d,I:%d", u8index, bFirstCalled[u8index]);

	if (bFirstCalled[u8index])
	{
		bFirstCalled[u8index] = FALSE;
	}
}

PUBLIC void DriverBulb_vOn(uint8 u8index)
{
	DriverBulb_vSetOnOff(u8index, TRUE);
}

PUBLIC void DriverBulb_vOff(uint8 u8index)
{
	DriverBulb_vSetOnOff(u8index, FALSE);
}

PUBLIC void DriverBulb_vSetOnOff(uint8 u8index, bool_t bOn)
{
     bBulbOn[u8index] =  bOn;
     DBG_vPrintf(TRUE, "\n#%d,S:%d", u8index, bOn);
}

PUBLIC void DriverBulb_vSetLevel(uint8 u8index, uint32 u32Level)
{
	DBG_vPrintf(TRUE, "\n#%d,L:%d", u8index, u32Level);
}

PUBLIC void DriverBulb_vSetColour(uint8 u8index, uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	DBG_vPrintf(TRUE, "\n#%d,C:%d %d %d", u8index, u32Red,u32Green,u32Blue);
}

PUBLIC bool_t DriverBulb_bOn(uint8 u8index)
{
	return (bBulbOn[u8index]);
}

PUBLIC bool_t DriverBulb_bReady(uint8 u8index)
{
	return (TRUE);
}

PUBLIC bool_t DriverBulb_bFailed(uint8 u8index)
{
	return (FALSE);
}

PUBLIC void DriverBulb_vTick(uint8 u8index)
{
/* No timing behaviour needed in DR1175 */
}

PUBLIC int16 DriverBulb_i16Analogue(uint8 u8index, uint8 u8Adc, uint16 u16AdcRead)
{
	return(ADC_FULL_SCALE);
}

/* This function replicates the 'real bulb' set colour temperature function called */
/* on DR1221 drivers and allows the DR1175 to look like a CCT=TW bulb */
PUBLIC void DriverBulb_vSetTunableWhiteColourTemperature(uint8 u8index, int32 i32ColourTemperature)
{
#ifdef CCT
	uint16 u16Mireds;

	/* Convert passed in temperature to mireds (passed in as kelvins) */
	u16Mireds = (uint16) (1000000/i32ColourTemperature);

	/* Value in range ? */
	if (u16Mireds <= 1023)
	{
		uint8   u8Loop;
		uint32 u32RangeM = 0;
		uint32 u32DiffM;
		uint32 u32RangeRGB;
		uint32 u32DiffRGB;

		/* Start with all values set to full */
		uint8 u8Red   = 255;
		uint8 u8Green = 255;
		uint8 u8Blue  = 255;

		/* Loop through table until we find entry above current temperature */
		for (u8Loop = 1; u8Loop < 17 && u32RangeM == 0; u8Loop++)
		{
			/* Is this the value above the one we are looking for ? */
			if (au32Mireds2RGB[u8Loop][0] > u16Mireds)
			{
				/* Calculate range and difference in mireds */
				u32RangeM = au32Mireds2RGB[u8Loop][0] - au32Mireds2RGB[u8Loop-1][0];
				u32DiffM  = u16Mireds                 - au32Mireds2RGB[u8Loop-1][0];
				/* Calculate red */
				u32RangeRGB = au32Mireds2RGB[u8Loop][1] - au32Mireds2RGB[u8Loop-1][1];
				u32DiffRGB  = (u32DiffM * u32RangeRGB) / u32RangeM;
				u8Red   = au32Mireds2RGB[u8Loop-1][1] + u32DiffRGB;
				/* Calculate green */
				u32RangeRGB = au32Mireds2RGB[u8Loop][2] - au32Mireds2RGB[u8Loop-1][2];
				u32DiffRGB  = (u32DiffM * u32RangeRGB) / u32RangeM;
				u8Green = au32Mireds2RGB[u8Loop-1][2] + u32DiffRGB;
				/* Calculate blue */
				u32RangeRGB = au32Mireds2RGB[u8Loop][3] - au32Mireds2RGB[u8Loop-1][3];
				u32DiffRGB  = (u32DiffM * u32RangeRGB) / u32RangeM;
				u8Blue = au32Mireds2RGB[u8Loop-1][3] + u32DiffRGB;
			}
		}

		bRGB_LED_SetLevel(u8Red, u8Green, u8Blue);
	}
#endif
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
