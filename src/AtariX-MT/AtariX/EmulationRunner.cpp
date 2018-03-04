/*
 * Copyright (C) 1990-2018 Andreas Kromke, andreas.kromke@gmail.com
 *
 * This program is free software; you can redistribute it or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 *  EmulationRunner.cpp
 *
 */

#include <assert.h>
#include "EmulationRunner.h"


/*********************************************************************************************************
*
* Constructor
*
*********************************************************************************************************/

EmulationRunner::EmulationRunner(void)
{
	printf("%s()\n", __func__);
    m_bQuitLoop = false;
    //drawContext = NULL;
	m_EmulatorThread = NULL;
	m_200HzCnt = 0;
	m_EmulatorRunning = false;

	// default values
	
	m_atariScreenW = 1024;
	m_atariScreenH = 768;
	m_atariScreenStretchX = m_atariScreenStretchY = false;
	m_atariHideHostMouse = false;
	screenbitsperpixel = 32;
}


/*********************************************************************************************************
*
* Destructor
*
*********************************************************************************************************/

EmulationRunner::~EmulationRunner(void)
{
	printf("%s()\n", __func__);
}


/*********************************************************************************************************
*
* Initialisation
*
* Does some default initialisations that are independent from the current setting/configuration.
*
*********************************************************************************************************/

void EmulationRunner::Init(void)
{
	printf("%s()\n", __func__);
    int ret;

	m_counter = 0;

	// we do not want SDL to catch events like SIGSEGV
	ret = SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	assert(!ret);
}


/*********************************************************************************************************
 *
 * Configuration
 *
 *********************************************************************************************************/

void EmulationRunner::Config
(
	const char *atariKernelPathUrl,
	const char *atariRootfsPathUrl,
	unsigned atariMemorySize,
	unsigned atariScreenWidth,
	unsigned atariScreenHeight,
	unsigned atariScreenColourMode,
	bool atariScreenStretchX,
	bool atariScreenStretchY,
	unsigned atariLanguage,
	bool atariHideHostMouse,
	const char *atariPrintCommand,
	const char *atariSerialDevice
)
{
	printf("%s()\n", __func__);

	// memory size is passed as Megabytes (2 ^ 20)
	if (atariMemorySize < 1)
		atariMemorySize = 1;
	else
	if (atariMemorySize > MAX_ATARIMEMSIZE >> 20)
		atariMemorySize = MAX_ATARIMEMSIZE >> 20;
	Globals.s_Preferences.m_AtariMemSize = atariMemorySize << 20;

	if (atariScreenWidth < 320)
		atariScreenWidth = 320;
	else
	if (atariScreenWidth > 4096)
		atariScreenWidth = 4096;
	m_atariScreenW = atariScreenWidth;

	if (atariScreenHeight < 200)
		atariScreenHeight = 200;
	else
	if (atariScreenHeight > 2048)
		atariScreenHeight = 2048;
	m_atariScreenH = atariScreenHeight;

	if (atariScreenColourMode == 6)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode2;
	else
	if (atariScreenColourMode == 5)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode4ip;
	else
	if (atariScreenColourMode == 4)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode16ip;
	else
	if (atariScreenColourMode == 3)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode16;
	else
	if (atariScreenColourMode == 2)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode256;
	else
	if (atariScreenColourMode == 1)
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenModeHC;
	else
		Globals.s_Preferences.m_atariScreenColourMode = atariScreenMode16M;

	printf("%s(): atariScreenColourMode (%u)\n", __func__, atariScreenColourMode);

	m_atariScreenStretchX = atariScreenStretchX;
	m_atariScreenStretchY = atariScreenStretchY;
	printf("%s(): atariHideHostMouse(%u) -- ignored, because unreliable in SDL\n", __func__, atariHideHostMouse);
//	m_atariHideHostMouse  = atariHideHostMouse;

	if ((atariPrintCommand) && strlen(atariPrintCommand) < 255)
	{
		strcpy(Globals.s_Preferences.m_szPrintingCommand, atariPrintCommand);
	}
	else
	{
		printf("%s(): atariPrintCommand string empty or too long, ignored\n", __func__);
	}

	if ((atariSerialDevice) && strlen(atariSerialDevice) < 255)
	{
		strcpy(Globals.s_Preferences.m_szAuxPath, atariSerialDevice);
	}
	else
	{
		printf("%s(): atariSerialDevice string empty or too long, ignored\n", __func__);
	}

	if ((atariKernelPathUrl) && strlen(atariKernelPathUrl) < 1024)
	{
		strcpy((char *) Globals.s_atariKernelPathUrl, atariKernelPathUrl);
	}
	else
	{
		printf("%s(): atariKernelPathUrl string empty or too long, ignored\n", __func__);
	}

	if ((atariRootfsPathUrl) && strlen(atariRootfsPathUrl) < 1024)
	{
		strcpy((char *) Globals.s_atariRootfsPathUrl, atariRootfsPathUrl);
	}
	else
	{
		printf("%s(): s_atariRootfsPathUrl string empty or too long, ignored\n", __func__);
	}
}


/*********************************************************************************************************
 *
 * Define virtual Atari drive
 *
 *********************************************************************************************************/

void EmulationRunner::ChangeAtariDrive(unsigned drvnr, CFURLRef drvUrl)
{
	printf("%s()\n", __func__);
	if (drvnr < NDRIVES)
	{
		Globals.s_Preferences.m_drvPath[drvnr] = drvUrl;
		if (m_EmulatorRunning)
		{
			m_Emulator.ChangeXFSDrive(drvnr);
		}
	}
}


/*********************************************************************************************************
 *
 * Start the 68k emulation thread
 *
 *********************************************************************************************************/

int EmulationRunner::StartEmulatorThread(void)
{
	printf("%s()\n", __func__);
	if (!m_EmulatorThread)
	{
		// Send user event to event loop
		SDL_Event event;
		
		event.type = SDL_USEREVENT;
		event.user.code = RUN_EMULATOR;
		event.user.data1 = 0;
		event.user.data2 = 0;
		
		SDL_PushEvent(&event);

		return 0;
	}
	else
		return 1;
}


/*********************************************************************************************************
 *
 * Open the Emulation window (asynchronous function)
 *
 *********************************************************************************************************/

int EmulationRunner::OpenWindow(void)
{
	printf("%s()\n", __func__);
	if (!m_sdl_window)
	{
		// Send user event to event loop
		SDL_Event event;
		
		event.type = SDL_USEREVENT;
		event.user.code = OPEN_EMULATOR_WINDOW;
		event.user.data1 = 0;
		event.user.data2 = 0;

		SDL_PushEvent(&event);

		return 0;
	}
	else
		return 1;
}


/*********************************************************************************************************
 *
 * (private) A rectangle in an SDL Surface has changed
 *
 *********************************************************************************************************/

static void UpdateTextureFromRect(SDL_Texture *txtu, const SDL_Surface *srf, const SDL_Rect *rect)
{
	int r;
	
	const uint8_t *pixels = (const uint8_t *) srf->pixels;
	if (rect)
	{
		pixels += rect->y * srf->pitch;
		pixels += rect->x * srf->format->BytesPerPixel;
	}
	r = SDL_UpdateTexture(txtu, rect, pixels, srf->pitch);
	if (r == -1)
	{
		fprintf(stderr, "ERR: SDL %s", SDL_GetError());
	}
}


/*********************************************************************************************************
 *
 * (private) Convert bitmap format from Atari native to host native
 *
 *********************************************************************************************************/

static void ConvertSurface
(
	const SDL_Surface *pSrc,
	SDL_Surface *pDst,
	const UInt32 palette[256],
	bool bStretchX, bool bStretchY
)
{
	unsigned x,y;
	const uint8_t *ps8 = (const uint8_t *) pSrc->pixels;
	uint8_t *pd8 = (uint8_t *) pDst->pixels;
	const uint8_t *ps8x;
	uint32_t *pd32x;
	uint8_t c;
	// 0xff000000		black
	// 0xffff0000		red
	// 0xff00ff00		green
	// 0xff0000ff		blue
	uint32_t col0 = palette[0];
	uint32_t col1 = palette[1];
	// convert from RGB555 to RGB888 using a conversion table
	const uint32_t rgbConvTable5to8[32] =
	{
		0,
		( 1 * 255)/31,
		( 2 * 255)/31,
		( 3 * 255)/31,
		( 4 * 255)/31,
		( 5 * 255)/31,
		( 6 * 255)/31,
		( 7 * 255)/31,
		( 8 * 255)/31,
		( 9 * 255)/31,
		(10 * 255)/31,
		(11 * 255)/31,
		(12 * 255)/31,
		(13 * 255)/31,
		(14 * 255)/31,
		(15 * 255)/31,
		(16 * 255)/31,
		(17 * 255)/31,
		(18 * 255)/31,
		(19 * 255)/31,
		(20 * 255)/31,
		(21 * 255)/31,
		(22 * 255)/31,
		(23 * 255)/31,
		(24 * 255)/31,
		(25 * 255)/31,
		(26 * 255)/31,
		(27 * 255)/31,
		(28 * 255)/31,
		(29 * 255)/31,
		(30 * 255)/31,
		255
	};

	// hack to detect interleaved plane format
	int bitsperpixel = pSrc->format->BitsPerPixel;
	if (pSrc->userdata == (void *) 1)
		bitsperpixel *= 10;

	switch(bitsperpixel)
	{
		case 1:

			// monochrome, driver MFM2.SYS.
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line

				for (x = 0; x < pSrc->w; x += 8)
				{
					c = *ps8x++;		// get one byte, 8 pixels
					*pd32x++ = (c & 0x80) ? col1 : col0;
					*pd32x++ = (c & 0x40) ? col1 : col0;
					*pd32x++ = (c & 0x20) ? col1 : col0;
					*pd32x++ = (c & 0x10) ? col1 : col0;
					*pd32x++ = (c & 0x08) ? col1 : col0;
					*pd32x++ = (c & 0x04) ? col1 : col0;
					*pd32x++ = (c & 0x02) ? col1 : col0;
					*pd32x++ = (c & 0x01) ? col1 : col0;
				}

/* Let SDL do that in EmulatorWindowUpdate()
				if (bStretchY)
				{
					//copy line
					memcpy(pd8 + pDst->pitch, pd8, pDst->w * sizeof(uint32_t));
					pd8 += pDst->pitch;
				}
*/
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

		case 20:

			// 4 colours, organized as interleaved plane (16-bit-big-endian, lowest bit first), driver MFM4IP.SYS. ?????
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line
				uint16_t index3, index2, index1, index0;
				
				for (x = 0; x < pSrc->w; x += 16)
				{
					int i;
					uint8_t ca[16];

					index1 = *ps8x++;		// get one byte, bit 0 of first 8 pixels
					index1 <<= 0;
					index0 = *ps8x++;		// get one byte, bit 0 of  next 8 pixels
					index0 <<= 0;
					index3 = *ps8x++;		// get one byte, bit 1 of first 8 pixels
					index3 <<= 1;
					index2 = *ps8x++;		// get one byte, bit 1 of  next 8 pixels
					index2 <<= 1;
					for (i = 7; i >= 0; i--)
					{
						ca[i] = (index3 & 2) | (index1 & 1);
						index3 >>= 1;
						index1 >>= 1;
					}
					
					for (i = 15; i >= 8; i--)
					{
						ca[i] = (index2 & 2) | (index0 & 1);
						index2 >>= 1;
						index0 >>= 1;
					}
					
					for (i = 0; i < 16; i++)
					{
						// indexed colour, we must access the palette table here
						*pd32x++ = palette[ca[i]];
					}
				}

				/* Let SDL do that in EmulatorWindowUpdate()
				if (bStretchY)
				{
					//copy line
					memcpy(pd8 + pDst->pitch, pd8, pDst->w * sizeof(uint32_t));
					pd8 += pDst->pitch;
				}
				*/
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

		case 4:

			// 16 colours, organized as packed pixels, driver MFM16.SYS.
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line
				uint8_t index1, index0;
				
				for (x = 0; x < pSrc->w; x += 2)
				{
					index1 = *ps8x++;		// get one byte, 2 pixels (aaaabbbb)
					index0 = index1 >> 4;
					index1 &= 0x0f;
					
					// indexed colour, we must access the palette table here
		//			*pd32x++ = (index0 << 4) | (index0 << 12L) | (index0 << 20L) | (0xff000000);
					*pd32x++ = palette[index0];
		//			*pd32x++ = (index1 << 4) | (index1 << 12L) | (index1 << 20L) | (0xff000000);
					*pd32x++ = palette[index1];
				}
				
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

			/* The Atari ST however has interleaved bitplanes: The first word
			 of graphics memory describes the first 16 pixels on screen in
			 the first bitplane, the second word describes the same 16 pixels
			 in the second bitplane and so forth. If the Atari ST displays 16
			 colours on screen, meaning 4 bitplanes, you have 8 bytes (4 words)
			 of data which describe 16 pixels in all 4 bitplanes. */

		case 40:

			// 16 colours, organized as interleaved plane (16-bit-big-endian), driver MFM16IP.SYS.
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line
				uint16_t index31, index21, index11, index01;
				uint16_t index32, index22, index12, index02;

				for (x = 0; x < pSrc->w; x += 16)
				{
					int i;
					uint8_t ca[16];

					index01 = *ps8x++;		// get one byte, bit 0 of first 8 pixels
					index01 <<= 0;
					index02 = *ps8x++;		// get one byte, bit 0 of next 8 pixels
					index02 <<= 0;
					index11 = *ps8x++;		// get one byte, bit 1 of first 8 pixels
					index11 <<= 1;
					index12 = *ps8x++;		// get one byte, bit 1 of next 8 pixels
					index12 <<= 1;
					index21 = *ps8x++;		// get one byte, bit 2 of first 8 pixels
					index21 <<= 2;
					index22 = *ps8x++;		// get one byte, bit 2 of next 8 pixels
					index22 <<= 2;
					index31 = *ps8x++;		// get one byte, bit 3 of first 8 pixels
					index31 <<= 3;
					index32 = *ps8x++;		// get one byte, bit 3 of next 8 pixels
					index32 <<= 3;

					for (i = 7; i >= 0; i--)
					{
						ca[i] = (index31 & 8) | (index21 & 4) | (index11 & 2) | (index01 & 1);
						index31 >>= 1;
						index21 >>= 1;
						index11 >>= 1;
						index01 >>= 1;
					}

					
					for (i = 15; i >= 8; i--)
					{
						ca[i] = (index32 & 8) | (index22 & 4) | (index12 & 2) | (index02 & 1);
						index32 >>= 1;
						index22 >>= 1;
						index12 >>= 1;
						index02 >>= 1;
					}

					for (i = 0; i < 16; i++)
					{
						// indexed colour, we must access the palette table here
						*pd32x++ = palette[ca[i]];
					}
				}
				
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

		case 8:

			// 256 colours, organized as packed pixels, driver MFM256.SYS.
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line
				uint8_t index0;
				
				for (x = 0; x < pSrc->w; x++)
				{
					index0 = *ps8x++;		// get one byte, 1 pixel
					*pd32x++ = palette[index0];
				}
				
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

		case 16:

			// 32768 colours, organized as packed pixels, driver MFM32K.SYS.
			for (y = 0; y < pSrc->h; y++)
			{
				ps8x = ps8;					// pointer to source line
				pd32x = (uint32_t *) pd8;	// pointer to dest line
				uint32_t r,g,b,w;

				for (x = 0; x < pSrc->w; x++)
				{
					w = *ps8x++;		// get upper byte of pixel
					w <<= 8;
					w |= *ps8x++;		// get lower byte of pixel

					// extract colours
					r = (w >> 10) & 0x1f;
					g = (w >>  5) & 0x1f;
					b = (w >>  0) & 0x1f;
					// expand from 5 to 8 bit
					r = rgbConvTable5to8[r];
					g = rgbConvTable5to8[g];
					b = rgbConvTable5to8[b];

					w = (0xff000000) | (r << 16) | (g << 8) | (b);
					*pd32x++ = w;
				}
				
				// advance to next line
				ps8 += pSrc->pitch;
				pd8 += pDst->pitch;
			}
			break;

	}
}


/*********************************************************************************************************
 *
 * (private) Start the 68k emulation thread
 *
 *********************************************************************************************************/

void EmulationRunner::_StartEmulatorThread(void)
{
	m_timer = SDL_AddTimer(5 /* 5 milliseconds, 200 Hz */, LoopTimer, this);
	m_EmulatorThread = SDL_CreateThread(_EmulatorThread, "EmulatorThread", this);
}


/*********************************************************************************************************
*
* Create all necessary surfaces and textures and open the emulation window
*
* See VDI source file SETUP.C for actual usage of the structure members, e.g. planeBytes == 2
* forces Atari compatibility format, i.e. interleaved plane. The corresponding drivers are called
* "MFMxxIP.SYS". Otherwise planeBytes is not used by VDI and may have any value.
*
* The guest drivers in particular (halfword = 16 bit):
*
* MFM16M.SYS		24 bit true colour, 32 bits per pixel, direct colour
* MFM256.SYS		256 colours, indexed, 8 bits per pixel
* MFM16.SYS			16 colours, indexed, 4 bits per pixel (packed, i.e. 2 pixel per byte)
* MFM16IP.SYS		16 colours, indexed, 4 bits per pixel (interleaved plane, i.e. 16 pixels per 4 halfwords)
* MFM4.SYS			this driver is addressed, but does not exist, unfortunately.
* MFM4IP.SYS		4 colours, indexed, 2 bits per pixel (interleaved plane, i.e. 16 pixels per 2 halfwords)
* MFM2.SYS			2 colours, indexed, 1 bit per pixel (interleaved plane and packed pixel is here the same)
*
*********************************************************************************************************/

void EmulationRunner::_OpenWindow(void)
{
	int ret;
	// SDL stuff
	Uint32 rmask = 0;
	Uint32 gmask = 0;
	Uint32 bmask = 0;
	Uint32 amask = 0;
	// Pixmap stuff
	short pixelType = 0;
	UInt32 planeBytes;
	short cmpCount = 3;
	short cmpSize = 8;

	switch(Globals.s_Preferences.m_atariScreenColourMode)
	{
		case atariScreenMode2:
			screenbitsperpixel = 1;		// monochrome
			planeBytes = 0;				// do not force Atari compatibiliy (interleaved plane) mode
			cmpCount = 1;
			cmpSize = 1;
			break;

		case atariScreenMode4ip:
			screenbitsperpixel = 2;		// 4 colours, indirect
			planeBytes = 2;				// change to 0 to force packed pixel instead of interleaved plane
			cmpCount = 1;
			cmpSize = 2;
			break;

		case atariScreenMode16:
			screenbitsperpixel = 4;		// 16 colours, indirect
			planeBytes = 0;				// packed pixel
			cmpCount = 1;
			cmpSize = 4;
			break;

		case atariScreenMode16ip:
			screenbitsperpixel = 4;		// 16 colours, indirect
			planeBytes = 2;				// force interleaved plane
			cmpCount = 1;
			cmpSize = 4;
			break;

		case atariScreenMode256:
			screenbitsperpixel = 8;		// 256 colours, indirect
			planeBytes = 0;				// do not force Atari compatibiliy (interleaved plane) mode
			cmpCount = 1;
			cmpSize = 8;
			break;

		case atariScreenModeHC:
			screenbitsperpixel = 16;	// 32768 colours, direct
			rmask = 0x7C00;
			gmask = 0x03E0;
			bmask = 0x001F;
			amask = 0x8000;
			pixelType = 16;							// RGBDirect, 0 would be indexed
			planeBytes = 0;
			break;

		default:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			rmask = 0xff000000;
			gmask = 0x00ff0000;
			bmask = 0x0000ff00;
			amask = 0x000000ff;
#else
			rmask = 0x000000ff;
			gmask = 0x0000ff00;
			bmask = 0x00ff0000;
			amask = 0xff000000;
#endif
			pixelType = 16;							// RGBDirect, 0 would be indexed
			planeBytes = 0;
			break;
	}

	sprintf(m_window_title, "Atari Emulation (%ux%ux%u%s)", m_atariScreenW, m_atariScreenH, screenbitsperpixel, (planeBytes == 2) ? "ip" : "");
	m_visible = false;
	m_initiallyVisible = false;


	// note that the SDL surface cannot distinguish between packed pixel and interleaved.
	m_hostScreenW = m_atariScreenW * (m_atariScreenStretchX ? 2 : 1);
	m_hostScreenH = m_atariScreenH * (m_atariScreenStretchY ? 2 : 1);

	m_sdl_atari_surface = SDL_CreateRGBSurface(
			0,	// no flags
			m_atariScreenW,
			m_atariScreenH,
			screenbitsperpixel,
			rmask,
			gmask,
			bmask,
			amask);
	assert(m_sdl_atari_surface);
	// hack to mark the surface as "interleaved plane"
	if (planeBytes == 2)
		m_sdl_atari_surface->userdata = (void *) 1;
	// we do not deal with the alpha channel, otherwise we always must make sure that each pixel is 0xff******
	SDL_SetSurfaceBlendMode(m_sdl_atari_surface, SDL_BLENDMODE_NONE);

	// In case the Atari does not run in native host graphics mode, we need a converversion surface,
	// and instead of directly updating the texture from the Atari surface, we first convert it to 32 bits per pixel.

	if (screenbitsperpixel != 32)
	{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif
		m_sdl_surface = SDL_CreateRGBSurface(
											 0,	// no flags
											 m_hostScreenW,
											 m_hostScreenH,
											 32,
											 rmask,
											 gmask,
											 bmask,
											 amask);
		assert(m_sdl_surface);
		// we do not deal with the alpha channel, otherwise we always must make sure that each pixel is 0xff******
		SDL_SetSurfaceBlendMode(m_sdl_surface, SDL_BLENDMODE_NONE);
	}
	else
	{
		m_sdl_surface = m_sdl_atari_surface;
	}
	
	m_sdl_window = SDL_CreateWindow(
									m_window_title,
									100,
									100,
									m_hostScreenW,
									m_hostScreenH,
									SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	assert(m_sdl_window);

	//SDL_ShowWindow(m_sdl_window);  geht nicht, Fenster geht immer.

	m_sdl_renderer = SDL_CreateRenderer(m_sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (!m_sdl_renderer)
	{
		fprintf(stderr, "ERR: SDL %s\n", SDL_GetError());
	}
	assert(m_sdl_renderer);

	(void) SDL_FillRect(m_sdl_surface, NULL, 0x00ffffff);
	m_sdl_texture = SDL_CreateTextureFromSurface(m_sdl_renderer, m_sdl_surface);	// seems not to work with non-native format?!?
	assert(m_sdl_texture);

	// Clear the entire screen to our selected color (white).
	//	SDL_SetRenderDrawColor(m_sdl_renderer, 255, 255, 255, 255);
	//	SDL_RenderClear(m_sdl_renderer);

	// alles wei§, aber noch nicht rendern, weil Fenster unsichtbar.

	// draw test
#if 1
	SDL_Rect r = { 1, 1, 256, 256 };
	//SDL_Rect r = { 0, 0, screenw, screenh };
	// 0xff000000		black
	// 0xffff0000		red
	// 0xff00ff00		green
	// 0xff0000ff		blue
	ret = SDL_FillRect(m_sdl_surface, &r, 0x88888888);
	if (ret == -1)
	{
		fprintf(stderr, "ERR: SDL %s\n", SDL_GetError());
		//exit(-1);
	}
	UpdateTextureFromRect(m_sdl_texture, m_sdl_surface, &r);
#endif
//	UpdateTextureFromRect(m_sdl_texture, m_sdl_surface, NULL);

	/*
	 * Stuff needed for the MagiC graphics kernel
	 */

	// create ancient style Pixmap structure to be passed to the Atari kernel, from m_sdl_surface
	assert(m_sdl_atari_surface->pitch < 0x4000);			// Pixmap limit and thus limit for Atari
	assert((m_sdl_atari_surface->pitch & 3) == 0);		// pitch (alias rowBytes) must be dividable by 4
	
	MXVDI_PIXMAP *pixmap = &m_EmulatorScreen.m_PixMap;
	
	pixmap->baseAddr      = (UINT8 *) m_sdl_atari_surface->pixels;		// target address, filled in by emulator
	pixmap->rowBytes      = m_sdl_atari_surface->pitch | 0x8000;	// 0x4000 and 0x8000 are flags
	pixmap->bounds_top    = 0;
	pixmap->bounds_left   = 0;
	pixmap->bounds_bottom = m_sdl_atari_surface->h - 1;
	pixmap->bounds_right  = m_sdl_atari_surface->w - 1;
	pixmap->pmVersion     = 4;							// should mean: pixmap base address is 32-bit address
    pixmap->packType      = 0;							// unpacked?
    pixmap->packSize      = 0;							// unimportant?
    pixmap->pixelType     = pixelType;					// 16 is RGBDirect, 0 would be indexed
    pixmap->pixelSize     = m_sdl_atari_surface->format->BitsPerPixel;
    pixmap->cmpCount      = cmpCount;					// components: 3 = red, green, blue, 1 = monochrome
    pixmap->cmpSize       = cmpSize;					// True colour: 8 bits per component
	pixmap->planeBytes    = planeBytes;					// offset to next plane
	pixmap->pmTable       = NULL;
	pixmap->pmReserved    = NULL;
}


/*********************************************************************************************************
 *
 * (private static) timer function
 *
 *********************************************************************************************************/

Uint32 EmulationRunner::LoopTimer(Uint32 interval, void *param)
{
	EmulationRunner *p = (EmulationRunner *) param;

	if (p->m_EmulatorRunning)
	{
		p->m_Emulator.SendHz200();
		p->m_200HzCnt++;
		if ((p->m_200HzCnt % 4) == 0)
		{
			// VBL interrupt runs with 50 Hz
			p->m_Emulator.SendVBL();
		}

		if (((p->m_200HzCnt % 8) == 0) && (p->m_Emulator.bVideoBufChanged))
		{
			// screen update runs with 25 Hz

			// Create a user event to call the game loop.
			SDL_Event event;
			
			event.type = SDL_USEREVENT;
			event.user.code = RUN_EMULATOR_WINDOW_UPDATE;
			event.user.data1 = 0;
			event.user.data2 = 0;

			SDL_PushEvent(&event);
		}
	}

    return interval;
}


/*********************************************************************************************************
 *
 * Cleanup
 *
 *********************************************************************************************************/

void EmulationRunner::Cleanup(void)
{
    (void) SDL_RemoveTimer(m_timer);
    SDL_Quit();
}


/*********************************************************************************************************
 *
 * Debug helper
 *
 *********************************************************************************************************/

const char *SDL_WindowEventID_to_str(SDL_WindowEventID id)
{
	switch(id)
	{
		case SDL_WINDOWEVENT_NONE:			return "NONE";
		case SDL_WINDOWEVENT_SHOWN:			return "SHOWN";
		case SDL_WINDOWEVENT_HIDDEN:		return "HIDDEN";
		case SDL_WINDOWEVENT_EXPOSED:		return "EXPOSED";
		case SDL_WINDOWEVENT_MOVED:			return "MOVED";
		case SDL_WINDOWEVENT_RESIZED:		return "RESIZED";
		case SDL_WINDOWEVENT_SIZE_CHANGED:	return "SIZE_CHANGED";
		case SDL_WINDOWEVENT_MINIMIZED:		return "MINIMIZED";
		case SDL_WINDOWEVENT_MAXIMIZED:		return "MAXIMIZED";
		case SDL_WINDOWEVENT_RESTORED:		return "RESTORED";
		case SDL_WINDOWEVENT_ENTER:			return "ENTER";
		case SDL_WINDOWEVENT_LEAVE:			return "LEAVE";
		case SDL_WINDOWEVENT_FOCUS_GAINED:	return "FOCUS_GAINED";
		case SDL_WINDOWEVENT_FOCUS_LOST:	return "FOCUS_LOST";
		case SDL_WINDOWEVENT_CLOSE:			return "CLOSE";
	}

	return "UNKNOWN";
}


/*********************************************************************************************************
*
* SDL event loop. Must most probably be run in main thread (GUI thread)?!?
*
*********************************************************************************************************/

void EmulationRunner::EventLoop(void)
{
	uint8_t *clipboardData;

	printf("%s()\n", __func__);
    SDL_Event event;

	// Do not catch keyboard events, leave them for dialogue windows
	SDL_KeyboardActivate(0);		// TODO: Find better hack

    while((!m_bQuitLoop) && (SDL_WaitEvent(&event)))
	{
        switch(event.type)
		{
			case SDL_WINDOWEVENT:
				{
					const SDL_WindowEvent *ev = (SDL_WindowEvent *) &event;
					fprintf(stderr, "INF: SDL window event: evt=%u, wid=%u, ev=%s, data1=0x%08x, data2=0x%08x\n",
							ev->type,
							ev->windowID,
							SDL_WindowEventID_to_str((SDL_WindowEventID) ev->event),
							ev->data1,
							ev->data2);
					switch(ev->event)
					{
						case SDL_WINDOWEVENT_SHOWN:
							m_visible = true;
							m_initiallyVisible = true;
							break;

						case SDL_WINDOWEVENT_FOCUS_GAINED:
							// TODO: Copy Mac Clipboard to Atari Clipboard
							if (SDL_HasClipboardText())
							{
								// get clipboard text from SDL, hopefully in UTF-8
								clipboardData = (uint8_t *) SDL_GetClipboardText();
								if (clipboardData)
								{
									CClipboard::Mac2Atari(clipboardData);
									SDL_free(clipboardData);
								}
							}
							// Now catch keyboard events
							SDL_KeyboardActivate(1);
							if (m_atariHideHostMouse)
							{
								SDL_ShowCursor(SDL_DISABLE);
							}
							break;

						case SDL_WINDOWEVENT_FOCUS_LOST:
							// TODO: Copy Atari Clipboard to Mac Clipboard
							clipboardData = NULL;
							CClipboard::Atari2Mac(&clipboardData);
							if (clipboardData)
							{
								SDL_SetClipboardText((const char *) clipboardData);
								free(clipboardData);
							}
							// No longer catch keyboard events
							SDL_KeyboardActivate(0);
							if (m_atariHideHostMouse)
							{
								// show mouse pointer
								SDL_ShowCursor(SDL_ENABLE);
							}
							break;
					}
				}
				break;

            case SDL_USEREVENT:
                HandleUserEvents(&event);
                break;
                
            case SDL_KEYDOWN:
            case SDL_KEYUP:
				{
					const SDL_KeyboardEvent *ev = (SDL_KeyboardEvent *) &event;
					fprintf(stderr, "INF: type %s\n", ev->type == SDL_KEYUP ? "up" : "down");
					fprintf(stderr, "INF: state %s\n", ev->state == SDL_PRESSED ? "pressed" : "released");
					fprintf(stderr, "INF: scancode = %08x, keycode = %08x, mod = %04x\n", ev->keysym.scancode, ev->keysym.sym, ev->keysym.mod);
					(void) m_Emulator.SendSdlKeyboard(ev->keysym.scancode, ev->type == SDL_KEYUP);
				}
                // Quit when user presses a key.
                //m_bQuitLoop = true;
				//showPreferencesWindow();
                break;

			case SDL_MOUSEMOTION:
				{
					const SDL_MouseMotionEvent *ev = (SDL_MouseMotionEvent *) &event;
					fprintf(stderr, "INF: mouse motion x = %d, y = %d, xrel = %d, yrel = %d\n", ev->x, ev->y, ev->xrel, ev->yrel);
					int x = ev->x;
					int y = ev->y;
					if (m_atariScreenStretchX)
						x /= 2;
					if (m_atariScreenStretchY)
						y /= 2;
					m_Emulator.SendMousePosition(x, y);
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					const SDL_MouseButtonEvent *ev = (SDL_MouseButtonEvent *) &event;
					fprintf(stderr, "INF: mouse button %s: x = %d, y = %d, button = %d\n",
							ev->type == SDL_MOUSEBUTTONUP ? "up" : "down", ev->x, ev->y, ev->button);
					int atariMouseButton = -1;
					if (ev->button == 1)
						atariMouseButton = 0;
					else
					if (ev->button == 3)
						atariMouseButton = 1;
					if (atariMouseButton >= 0)
					{
						// left button is 0, right button is 1
						m_Emulator.SendMouseButton(atariMouseButton, ev->type == SDL_MOUSEBUTTONDOWN);
					}
				}
				break;

			case SDL_MOUSEWHEEL:
				{
					const SDL_MouseWheelEvent *ev = (SDL_MouseWheelEvent *) &event;
					fprintf(stderr, "INF: mouse wheel: x = %d, y = %d\n", ev->x, ev->y);
				}
				break;

			case SDL_TEXTEDITING:
				break;

			case SDL_TEXTINPUT:
				break;

            case SDL_QUIT:
				m_Emulator.TerminateThread();
                m_bQuitLoop = true;
                break;

            default:
				fprintf(stderr, "WRN: unhandled SDL event %u\n", event.type);
                break;
        }   // End switch
            
    }   // End while

	printf("%s() =>\n", __func__);
}


/*********************************************************************************************************
*
* SDL event loop: User events
*
*********************************************************************************************************/

void EmulationRunner::HandleUserEvents(SDL_Event* event)
{
    switch (event->user.code)
	{
		case OPEN_EMULATOR_WINDOW:
			if (!m_sdl_window)
			{
				_OpenWindow();
			}
			break;

		case RUN_EMULATOR:
			if (!m_EmulatorThread)
			{
				_StartEmulatorThread();
			}
			break;

        case RUN_EMULATOR_WINDOW_UPDATE:
			if (m_sdl_window)
			{
				EmulatorWindowUpdate();
			}
            break;

        default:
            break;
    }
}


/*********************************************************************************************************
*
* SDL event loop: Special user event to update the emulation window
*
*********************************************************************************************************/

void EmulationRunner::EmulatorWindowUpdate(void)
{
	// also does stretching:
	SDL_Rect rc = { 0, 0, (int) m_hostScreenW, (int) m_hostScreenH };		// dst
	SDL_Rect rc2 = { 0, 0, (int) m_atariScreenW, (int) m_atariScreenH };	// src
	if (OSAtomicTestAndClear(0, &m_Emulator.bVideoBufChanged))
	{
		fprintf(stderr, "INF: Atari Screen dirty\n");
		if (m_sdl_atari_surface != m_sdl_surface)
		{
			ConvertSurface(m_sdl_atari_surface, m_sdl_surface, m_EmulatorScreen.m_pColourTable, m_atariScreenStretchX, m_atariScreenStretchY);
		}

		UpdateTextureFromRect(m_sdl_texture, m_sdl_surface, NULL);

		if (m_visible)
		{
			if (m_initiallyVisible)
			{
				(void) SDL_RenderCopy(m_sdl_renderer, m_sdl_texture, NULL, NULL);
				m_initiallyVisible = false;
			}
			else
			{
				(void) SDL_RenderCopy(m_sdl_renderer, m_sdl_texture, &rc2, &rc);
				//SDL_GL_SwapWindow(m_sdl_window);
			}
			SDL_RenderPresent(m_sdl_renderer);
		}
	}
}


/*********************************************************************************************************
*
* thread starter helper
*
*********************************************************************************************************/

/* static */ int EmulationRunner::_EmulatorThread(void *ptr)
{
	EmulationRunner *pThis = (EmulationRunner *) ptr;
	return pThis->EmulatorThread();
}


/*********************************************************************************************************
*
* start emulator. This thread automatically will die after having done so.
*
*********************************************************************************************************/

int EmulationRunner::EmulatorThread()
{
	printf("%s()\n", __func__);
	int err;

	DebugInit(NULL /* stderr */);
	err = CGlobals::Init();
	if (err)
	{
		fprintf(stderr, "ERR: CGlobals::Init() => %d\n", err);
		return 0;
	}

	err = m_Emulator.Init(&m_EmulatorScreen, &m_EmulatorXcmd);
	if (err)
	{
		fprintf(stderr, "ERR: m_Emulator.Init() => %d\n", err);
		return 0;
	}

	err = m_Emulator.CreateThread();
	if (err)
	{
		fprintf(stderr, "ERR: m_Emulator.CreateThread() => %d\n", err);
		return 0;
	}

	m_EmulatorRunning = true;

	m_Emulator.StartExec();

	printf("%s() =>\n", __func__);
	return 0;
}
