/*
 * typemapper.h - non-32bit CPU and miscelany utilities
 *
 * Copyright (c) 2001-2003 STanda of ARAnyM developer team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TYPEMAPPER_H_INCLUDED__
#define __TYPEMAPPER_H_INCLUDED__ 1

#include <map>

// enables the conversions also on the 32bit system
//    (in case the system doesn't need them)
#define DEBUG_FORCE_NON32BIT 0



// single define to force 32bit algorithms
#if DEBUG_FORCE_NON32BIT
#define DEBUG_NON32BIT 1
#else
#define DEBUG_NON32BIT 0
#endif


/**
 * Provides bijective mapping between uint32_t and e.g. void*
 *   (or int or any other type)
 *
 * It is a need when there is not enough space on the emulated
 * side and we need to store native pointers there (hostfs)
 *
 * Also the filedescriptor number is int (which is not always 32bit)
 * and therefore we need to handle them this way.
 */
template <typename nativeType, typename atariType>
class NativeTypeMapper
{
	std::map<atariType, nativeType> a2n;
	std::map<nativeType, atariType> n2a;

  public:
	atariType putNative(nativeType value)
	{
		typename std::map<nativeType, atariType>::iterator it = n2a.find(value);

		// test if present
		if (it != n2a.end())
			return it->second;

		// cast to the number (not a pointer) type
		// of the same size as the void*. Then cut the lowest
		// 32bits as the default hash value
		atariType aValue = (atariType)(uintptr_t)value;

		// make the aValue unique (test if present and increase if positive)
		while (a2n.find(aValue) != a2n.end())
		{
#if DEBUG_FORCE_NON32BIT
			fprintf(stderr, "NTM: Conflicting mapping %lx [%ld]\n", (unsigned long)aValue, (long)a2n.size());
#endif
			aValue += 7;
		}

#if DEBUG_FORCE_NON32BIT
		fprintf(stderr, "NTM: mapping %p -> %lx [%ld]\n", (void *)value, (unsigned long)aValue, (long)a2n.size());
#endif
		// put the values into maps (both direction search possible)
		a2n.insert(std::make_pair(aValue, value));
		n2a.insert(std::make_pair(value, aValue));

		return aValue;
	}

	void removeNative(nativeType value)
	{
		typename std::map<nativeType, atariType>::iterator it = n2a.find(value);

		// remove if present
		if (it != n2a.end())
		{
			// remove the 32bit -> native mapping
			a2n.erase(it->second);
			// and now the native -> 32bit
			n2a.erase(value);
		}
	}

	nativeType getNative(atariType from)
	{
		return a2n.find(from)->second;
	}

	atariType get32bit(nativeType from)
	{
		return n2a.find(from)->second;
	}
};


// if the void* is not 4 byte long or if the map debugging is on
//    and if it is not explicitely turned off
#if __SIZEOF_POINTER__ > 4 || DEBUG_NON32BIT

# define MAPNEWVOIDP(x)   m_xfs.memptrMapper.putNative(x)
# define MAPDELVOIDP(x)   m_xfs.memptrMapper.removeNative(x)
# define MAP32TOVOIDP(x)  memptrMapper.getNative(x)
# define MAPVOIDPTO32(x)  memptrMapper.get32bit(x)

#else

# define MAPNEWVOIDP(x)   ((uint32_t)(x))
# define MAPDELVOIDP(x)
# define MAP32TOVOIDP(x)  x
# define MAPVOIDPTO32(x)  ((uint32_t)(x))

#endif


#endif /* __TYPEMAPPER_H_INCLUDED__ */
