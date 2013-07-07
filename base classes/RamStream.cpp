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

#include "RamStream.h"
#include <stdlib.h>
#include <string.h>

namespace AccessData
{

void CRamBlockStream::initfields()
{
	m_buffer = NULL;
	m_buffersize = 0;
}

void CRamBlockStream::clearfields()
{
	if ( m_buffer ) free(m_buffer);
	m_buffer = NULL;
	m_buffersize = 0;
}

void CRamBlockStream::assignfields(const CRamBlockStream &rhs)
{
	if ( rhs.m_buffer && rhs.m_buffersize )
	{
		m_buffersize = rhs.m_buffersize;
		m_buffer = (char *)malloc( m_buffersize );
		memcpy(m_buffer, rhs.m_buffer, m_buffersize);
	}
}

CRamBlockStream::CRamBlockStream()
{
	initfields();
}

CRamBlockStream::CRamBlockStream(const CRamBlockStream &rhs) : inherited(rhs)
{
	initfields();
	assignfields(rhs);
}

CRamBlockStream::~CRamBlockStream()
{
	clearfields();
}

bool CRamBlockStream::isvalid() const
{
	return m_buffer != NULL;
}

void CRamBlockStream::clear()
{
	inherited::clear();
	clearfields();
}

void CRamBlockStream::assign(const CRamBlockStream &rhs)
{
	inherited::assign(rhs);
	clearfields();
	assignfields(rhs);
}

CRamBlockStream &CRamBlockStream::operator=(const CRamBlockStream &rhs)
{
	assign(rhs);
	return *this;
}

void CRamBlockStream::setbuffer(const void *buffer, int buffersize)
{
	clearfields();
	if ( !buffer ) return;

	m_buffer = (char *)malloc(buffersize);
	if ( !m_buffer ) return;

	m_buffersize = buffersize;
	memcpy(m_buffer, buffer, m_buffersize);

    m_size = buffersize;
}


//
// CStream inherited
//
CStream *CRamBlockStream::Dup() const
{
	return new CRamBlockStream(*this);
}

int CRamBlockStream::Read(void *dest, int bytestoread, INT64 pos)
{
	if ( !isvalid() || !dest ) return -1;
	if ( pos > m_buffersize ) return 0;
	if ( pos+bytestoread > m_buffersize ) bytestoread = m_buffersize - pos;
	memcpy(dest, m_buffer+(int)pos, bytestoread);
	return bytestoread;
}

fssize_t CRamBlockStream::PhysicalLength() const
{
	return m_buffersize;
}

};		// end namespace
