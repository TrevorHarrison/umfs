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

#ifndef NTFSBITMAP_H
#define NTFSBITMAP_H

#include "ADIOTypes.h"
#include "ADStream.h"

namespace AccessData
{
namespace NTFS
{

class CNTFSBitmap
{
public:
	CNTFSBitmap();
	CNTFSBitmap(const CNTFSBitmap &rhs);
	~CNTFSBitmap();

	bool		isvalid() const;
	void		clear();

	// takes ownership of s
	bool		open(CStream *s, INT64 bitcount = -1);

	fssize_t	count() const;

	bool		operator[](fssize_t index);
	bool		getbit(fssize_t index);

	// returns the length of a run of bits that have the same value, starting at startbit
	// returns 0 if the end of the stream has been reached
	fssize_t	getrun(fssize_t startbit, fssize_t maxrun, bool &b);

    void		summary(INT64 &bits_set, INT64 &bits_unset);

    INT64		getfirst(bool b) { return getnext(b, 0); }
    INT64		getnext(bool b, INT64 startpos = 0);
protected:
	void		initfields();
	void		clearfields();
	void		assignfields(const CNTFSBitmap &rhs);

	bool		fillbuffer(fssize_t startpos);		// fill the cache buffer starting at byte startpos

	CStream*			m_stream;
	fssize_t			m_bitcount;
	UINT8*				m_cachebuffer;
	fssize_t			m_cachebufferposition;
	int					m_cachebufferlength;			// the number of valid bytes in the buffer
	int					m_cachebuffersize;				// the allocated size of the buffer

	enum { READAHEADBUFFERSIZE = 512 };
};

}		// end namespace NTFS
}		// end namespace AccessData

#endif
