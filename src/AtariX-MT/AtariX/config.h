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

//
//  config.h
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 07.10.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef SDLOpenGLIntro_config_h
#define SDLOpenGLIntro_config_h


#define EVENT_MOUSE 1

#if defined(USE_MUSASHI_68K_EMU)
// #define DEBUG_68K_EMU 1
#endif

// debug output for debug configuration
#if defined(_DEBUG)
//#define _DEBUG_BASEPAGE
//#define _DEBUG_KBD_AND_MOUSE
//#define _DEBUG_KB_CRITICAL_REGION
#define EMULATE_68K_TRACE 1
#define WATCH_68K_PC 1
#endif

// emulator kernel
#if __ppc__
// PPC can use either Asgard or Musashi
// default is Asgard (faster)
#if !defined(USE_MUSASHI_68K_EMU)
#define USE_ASGARD_PPC_68K_EMU 1
#define PATCH_VDI_PPC 1
#endif
#else
// i386 never uses Asgard
// i386 always uses Musashi
#undef USE_ASGARD_PPC_68K_EMU
#define USE_MUSASHI_68K_EMU 1
#endif

#define NAME_STRING "AtariX"
#define VERSION_STRING "0.3"

#if defined(__APPLE__)
#define OS_TYPE "macOS"
#define HOST_SCREEN_DRIVER "Quartz"
#elif defined(__ANDROID__)
#define OS_TYPE "Android"
#define HOST_SCREEN_DRIVER "X"
#elif defined(__linux__)
#define OS_TYPE "Linux"
#define HOST_SCREEN_DRIVER "X"
#elif defined(_WIN32) || defined(__CYGWIN__)
#define OS_TYPE "Windows"
#define HOST_SCREEN_DRIVER "GDI"
#else
#define OS_TYPE "Unknown"
#define HOST_SCREEN_DRIVER "Unknown"
#endif

#ifdef __x86_64__
#define HOST_CPU_TYPE "x86-64"
#elif defined(__i386__)
#define HOST_CPU_TYPE "x86"
#elif defined(__aarch64__)
#define HOST_CPU_TYPE "aarch64"
#elif defined(__arm__)
#define HOST_CPU_TYPE "arm"
#else
#define HOST_CPU_TYPE "Unknown"
#endif

#endif
