/*
	FILE NAME:

	FILE DESCRIPTION:

	CREDITS:

	--------------------------------------------------------------------------
	Copyright 2002, 2003 Trevor Harrison

	* This file is licensed under the GPL.  See LICENSE.TXT for details.
	* This file was given to Trevor Harrison by AccessData
	(www.accessdata.com) so that it could be released to the public under
	the GPL.  See ADLICENSE.TXT for details.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
*/

#ifndef INTTYPES_H
#define INTTYPES_H

//#ifdef _MSC_VER
	#include <basetsd.h>
//#endif

namespace AccessData
{

#ifdef __BORLANDC__
	// needs 2 underscores
	typedef          __int8  INT8;
	typedef unsigned __int8  UINT8;
	typedef          __int16 INT16;
	typedef unsigned __int16 UINT16;
	using ::UINT32;
	using ::INT32;
#endif

#ifdef _MSC_VER
	// needs 1 underscore
	typedef          _int16 INT16;
	typedef unsigned _int16 UINT16;
	typedef          _int8  INT8;
	typedef unsigned _int8  UINT8;
#endif

typedef INT64 fssize_t;

// Allows manipulation of individual bits of an int.
class CState
{
public:
	CState() : m_state(0) { }
	CState(const CState &rhs) : m_state(rhs.m_state) { }
	~CState() {}
	inline CState &operator=(const CState &rhs)		{ m_state = rhs.m_state; return *this; }
	inline bool operator==(const CState &rhs)		{ return m_state == rhs.m_state; }
	inline void clear() 							{ m_state = 0; }
	inline bool Is(int state)						{ return (state & m_state) ? true : false; }
	inline void Set(int state)						{ m_state |= state; }
	inline void Remove( int state)					{ m_state &= ~state; }
private:
	int		m_state;
};

struct FastBase2Int
{
	int		value;			// a number that is a simple power of 2
	int		mask;			// a bitmask used to quickly divide a number by .value
	int		bits;			// the number of bits to shift right to divide a number by .value

	FastBase2Int() : value(0), mask(0), bits(0) { }
	void clear() { value = 0; mask = 0; bits = 0; }

	void	Set(int i)		// set .value
	{
		value = i;
		mask = value - 1;
		bits = 0;
		while ( (i & 1) == 0 )
		{
			i >>= 1;
			bits++;
		}
	}

	int		Mul(int x)		{ return x << bits; }		// Multiply x

	INT64	Div(INT64 x)	{ return x >> bits; }		// Divide x
	int		Div(int x)		{ return x >> bits; }		// Divide x

	int		Mod(INT64 x)	{ return (int)(x & mask); }		// Get the modulus of x
	int		Mod(int x)		{ return x & mask; }		// Get the modulus of x
};

template<class T>
inline T ad_min(T a, T b)
{
	return a < b ? a : b;
}

template<class T>
inline T div_roundup(T a, T divisor)
{
	T x = a / divisor;
    if ( a % divisor != 0 ) x++;

    return x;
}

}		// end namespace AccessData


#endif
