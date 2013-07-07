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

#include "NTFSBitmap.h"

namespace AccessData
{
namespace NTFS
{

void CNTFSBitmap::initfields()
{
	m_stream = NULL;
	m_bitcount = 0;
	m_cachebuffer = NULL;
	m_cachebufferposition = -1;
	m_cachebufferlength = 0;
	m_cachebuffersize = 0;
}

void CNTFSBitmap::clearfields()
{
	if ( m_stream ) delete m_stream;
	m_stream = NULL;
	m_bitcount = 0;
	if ( m_cachebuffer ) free(m_cachebuffer);
	m_cachebuffer = NULL;
	m_cachebufferposition = -1;
	m_cachebufferlength = 0;
	m_cachebuffersize = 0;
}

void CNTFSBitmap::assignfields(const CNTFSBitmap &rhs)
{
	m_stream = rhs.m_stream ? rhs.m_stream->Dup() : NULL;
	m_bitcount = rhs.m_bitcount;

	m_cachebuffersize = rhs.m_cachebuffersize;
	m_cachebufferlength = rhs.m_cachebufferlength;
	m_cachebufferposition = rhs.m_cachebufferposition;
	m_cachebuffer = (UINT8 *)malloc(m_cachebuffersize);
	memcpy(m_cachebuffer, rhs.m_cachebuffer, m_cachebufferlength);
}

CNTFSBitmap::CNTFSBitmap()
{
	initfields();
}

CNTFSBitmap::~CNTFSBitmap()
{
	clearfields();
}

bool CNTFSBitmap::isvalid() const
{
	return m_stream != NULL;
}

void CNTFSBitmap::clear()
{
	clearfields();
}

bool CNTFSBitmap::open(CStream *s, INT64 bitcount)
{
	clear();

	if ( !s || !s->isvalid() ) return false;

	m_stream = s;
	m_bitcount = bitcount >= 0 ? bitcount : s->Length() * 8;

	m_cachebuffersize = READAHEADBUFFERSIZE;
	m_cachebuffer = (UINT8 *)malloc(m_cachebuffersize);
	if ( !m_cachebuffer ) { clear(); return false; }

	return true;
}

fssize_t CNTFSBitmap::count() const
{
	return m_bitcount;
}

bool CNTFSBitmap::operator[](fssize_t index)
{
	return getbit(index);
}

bool CNTFSBitmap::getbit(fssize_t index)
{
	if ( !isvalid() || index >= m_bitcount ) return false;

	fssize_t bitpos = index / 8;

	// Check to see if the byte that holds the requested bit is in the m_cachebuffer
	if ( !(m_cachebufferposition <= bitpos && bitpos < m_cachebufferposition+m_cachebufferlength) )
	{
		// if no, fill the buffer starting at the desired byte
		if ( !fillbuffer(bitpos) ) return false;
	}

	// calculate where our byte is in the buffer
	int byteoffset = bitpos - m_cachebufferposition;
	int bitnum = index % 8;

	// Return the requested bit value
	return ((m_cachebuffer[byteoffset] >> bitnum) & 1) == 1;
}

fssize_t CNTFSBitmap::getrun(fssize_t startbit, fssize_t maxrun, bool &b)
{
	if ( !isvalid() || startbit >= m_bitcount ) return 0;

	fssize_t runlen = 0;
	if ( (maxrun < 0) || (startbit+maxrun >= m_bitcount) ) maxrun = m_bitcount - startbit; //0x7fffffffffffffffL;	// set maxrun to max_int64

	UINT8 value, match8;
	bool first = true;
	while ( runlen < maxrun )
	{
		int startpos = startbit / 8;

		// Check to see if the byte that holds the requested bit is in the m_cachebuffer
		if ( !(m_cachebufferposition <= startpos && startpos < m_cachebufferposition+m_cachebufferlength) )
		{
			if ( !fillbuffer(startpos) ) break;
		}
		int curbyte = startpos - m_cachebufferposition;

		// Process the partial first byte
		if ( first ) 
		{
			int bitnum = startbit % 8;
			value = (m_cachebuffer[curbyte] >> bitnum) & 1;		// find out what the first bit is
			b = (value == 1);										// set the value of b (indicates weither this run is a 1 or 0 run
			match8 = value ? 0xFF : 0;								// set match8 to 0 or 255 to match entire bytes at once
			first = false;
			if ( bitnum != 0 )										// if we only want part of the first byte, do it here
			{
				for(int bn=bitnum; bn < 8; bn++)
				{
					if ( ((m_cachebuffer[curbyte] >> bn) & 1) != value ) return runlen;
					runlen++;
				}
				curbyte++;
			}
		}

		// grab all the bytes that have all their bits set to the value we want (ie. 0 or 255)
		for( ; (curbyte < m_cachebufferlength) && (m_cachebuffer[curbyte] == match8) && (runlen < maxrun); curbyte++, runlen+=8) { /* nada */ }

		// now get the left over bits at the end
		if ( (curbyte < m_cachebufferlength) && (runlen < maxrun) )
		{
			for(int bn=0; bn < 8; bn++)
			{
				if (( (m_cachebuffer[curbyte] >> bn) & 1) != value ) return runlen;
				runlen++;
			}
		}
		startbit = (m_cachebufferposition+m_cachebufferlength)*8;
	}
	return runlen;
}

void CNTFSBitmap::summary(INT64 &bits_set, INT64 &bits_unset)
{
	bits_set = bits_unset = 0;
	bool prev_b = false;
	for(INT64 bitnum = 0; bitnum < m_bitcount; )
    {
    	bool b;
    	INT64 runlen = getrun(bitnum, -1, b);
        if ( bitnum == 0 ) prev_b = !b;
        if ( prev_b == b )
        {
        	int x = 0;		// bad... should not happen.
        }
        if ( b ) bits_set += runlen; else bits_unset += runlen;
        bitnum += runlen;
        prev_b = b;
    }
}

INT64 CNTFSBitmap::getnext(bool b, INT64 startpos)
{
	bool bitvalue;
	INT64 runlen = getrun(startpos, -1, bitvalue);
    return bitvalue == b ? startpos : startpos + runlen; 
}

bool CNTFSBitmap::fillbuffer(fssize_t startpos)
{
	m_cachebufferposition = startpos;
	m_cachebufferlength = m_stream->Read(m_cachebuffer, m_cachebuffersize, m_cachebufferposition);
	if ( m_cachebufferlength <= 0 )
	{
		m_cachebufferposition = -1;
		return false;
	}
	return true;

}

}		// end namespace NTFS
}		// end namespace AccessData
