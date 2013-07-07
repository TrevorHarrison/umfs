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

#include "BlockStream.h"
#include "Logger.h"

#include <assert.h>

namespace AccessData
{

void CBlockStream::initfields()
{
	m_dev = NULL;
    //m_runs.clear();
	m_bc = 0;
	m_size = 0;
    m_blocksize = 0;
	m_initialoffset = 0;
	m_cp = 0;
}

void CBlockStream::clearfields()
{
	m_dev = NULL;
	m_runs.clear();
	m_bc = 0;
	m_size = 0;
    m_blocksize = 0;
	m_initialoffset = 0;
	m_cp = 0;
}

void CBlockStream::assignfields(const CBlockStream &rhs)
{
	m_dev = rhs.m_dev;
	m_runs = rhs.m_runs;
	m_bc = rhs.m_bc;
	m_size = rhs.m_size;
    m_blocksize = rhs.m_blocksize;
	m_initialoffset = rhs.m_initialoffset;
	m_cp = rhs.m_cp;
}

CBlockStream::CBlockStream()
{
	initfields();
}

CBlockStream::CBlockStream(const CBlockStream &rhs) : inherited(rhs)
{
	initfields();
	assignfields(rhs);
}

CBlockStream::~CBlockStream()
{
	clearfields();
}


bool CBlockStream::isvalid() const
{
	return m_dev != NULL;
}

void CBlockStream::clear()
{
	clearfields();
}

void CBlockStream::assign(const CBlockStream &rhs)
{
	//inherited::assign(rhs);
	clearfields();
	assignfields(rhs);
}

CBlockStream &CBlockStream::operator=(const CBlockStream &rhs)
{
	assign(rhs);
	return *this;
}

bool CBlockStream::AddRun(INT64 startblock, INT64 blockcount)
{
	runinfo temp;
	temp.logicalstart = m_bc;
	temp.physicalstart = startblock;
	temp.count = blockcount;

	m_runs.push_back( temp );

	m_bc += blockcount;

	return true;
}

bool CBlockStream::GetRunInfo(int runnum, INT64 &logicalstart, INT64 &physicalstart, INT64 &length)
{
	if ( runnum < 0 || runnum >= m_runs.size() ) return false;

    logicalstart = m_runs[runnum].logicalstart;
    physicalstart = m_runs[runnum].physicalstart;
    length = m_runs[runnum].count;

    return true;
}

void CBlockStream::SetDev(CFTKBlockDevice *dev)
{
	m_dev = dev;
    m_blocksize = m_dev->ftkbioBlockSize();
}

CFTKBlockDevice *CBlockStream::GetDev() const
{
	return m_dev;
}


void CBlockStream::SetLength(INT64 size)
{
	m_size = size;
}

void CBlockStream::SetInitialOffset(int initialoffset)
{
	m_initialoffset = initialoffset;
}


bool CBlockStream::MakeSubFile(CBlockStream *f, INT64 pos, INT64 count)
{
	if ( !isvalid() || !f ) return false;

    pos += m_initialoffset;
	INT64 ps = PhysicalLength();
	if ( pos > ps ) return false;
	if ( pos+count > ps ) count = ps - pos;

	f->clear();

	INT64 newofs = pos % m_blocksize;
	INT64 startb = pos / m_blocksize;
	INT64 newbc = div_roundup( count+newofs, (INT64)m_blocksize );

	int i, runcount = m_runs.size();

	// find the run that houses our starting block
	for(i=0; i < runcount && startb >= m_runs[i].count; startb -= m_runs[i].count, i++) { /*nada*/ }
	if ( i >= runcount ) return false;

	// add whole runs to f's runlist until we take all the blocks we need
	for(; i < runcount && newbc > 0; i++)
	{
		INT64 x = min(m_runs[i].count-startb, newbc); // ? m_runs[i].count-startb : newbc;
		f->AddRun( m_runs[i].physicalstart+startb, x );
		newbc -= x;
		startb = 0;
	}
	
	f->SetDev( m_dev );
	f->SetLength( count );
	f->SetInitialOffset( newofs );

	return true;
}

INT64 CBlockStream::GetBlock(INT64 pos) const
{
	if ( !isvalid() ) return -1;
	INT64 result;
    pos += m_initialoffset;
	return blocknumxlat( result, pos / m_blocksize) ? result : -1;
}

INT64 CBlockStream::GetBlock(INT64 pos, CFTKBlockDevice *dev) const
{
	if ( !isvalid() ) return -1;

	INT64 blocknum;
    pos += m_initialoffset;
	if ( !blocknumxlat( blocknum, pos / m_blocksize) ) return -1;

	return m_dev->ftkbioBlockNumTranslate(blocknum, dev);
}

//
// CBlockStream inherited
//

CStream *CBlockStream::Dup() const
{
	CBlockStream *temp = new CBlockStream(*this);
	return temp;
}

int CBlockStream::Read(void *dest, int bytestoread)
{
	int result = Read(dest, bytestoread, m_cp);
	if ( result > 0 ) m_cp += result;
	return result;
}

int CBlockStream::Read(void *dest, int bytestoread, INT64 pos)
{
	if ( !dest || !isvalid() ) return -1;

	int totalbytesread = 0;
	char *cdest = (char *)dest;

    // here pos is logical, ie. [0..m_size]
	if ( pos >= m_size ) return 0; // early exit if the pos is past the end of the file
	if ( pos < 0 ) return 0;		// early exit if the pos is negative
	if ( pos+bytestoread > m_size ) bytestoread = m_size-pos;  // adjust a read past the end of the file to stop at the end of the file

    // here pos is now physical after being adjusted by m_initialoffset
    pos += m_initialoffset;

	while ( bytestoread > 0 )
	{
		//INT64 apos = pos+m_initialoffset;				// adjusted pos... not needed anymore
		int startofs = pos % m_blocksize;				// offset from the beginning of this block... only the 1st block should be non-zero
		int b = ad_min(m_blocksize - startofs, bytestoread);// bytes to read out of this block, the min of bs-startofs or the number of bytes left to read
		INT64 blocknum;
		if ( !blocknumxlat(blocknum, pos / m_blocksize) ) break;	// map the block num from local sequential blocks into the device blocks

		if ( blocknum < 0 )								// if its a sparse block
		{
			memset(cdest, 0, b);
		}
		else
		{
			if ( !m_dev->ftkbioBlockRead( cdest, blocknum, startofs, b) ) break;
		}

		//memcpy( cdest, m_blockbuffer+startofs, b );

		pos += b;
		cdest += b;
		totalbytesread += b;
		bytestoread -= b;
	}

	return totalbytesread;
}

bool CBlockStream::Eof()
{
	return m_cp >= m_size;
}

INT64 CBlockStream::Seek(INT64 amount, SEEK_WHENCE whence)
{
	if ( !isvalid() ) return -1;

	switch ( whence )
	{
		case swSET: m_cp = amount; break;
		case swCUR: m_cp += amount; break;
		case swEND: m_cp = m_size + amount; break;
	}
//	if ( m_cp < 0 ) { m_cp = 0; return -1; }
	// if it's a bad offset, set m_cp to the max value so that an EOF
	// will get reported when the caller tries to read from the file
	if (m_cp < 0) { m_cp = m_size; }

	return m_cp;
}

INT64 CBlockStream::Tell() const
{
	return m_cp;
}

INT64 CBlockStream::Length() const
{
	return m_size;
}

INT64 CBlockStream::PhysicalLength() const
{
	return m_blocksize * m_bc;
}

int CBlockStream::BlockSize() const
{
	return m_blocksize;
}

int CBlockStream::PhysicalBlockSize() const
{
	return isvalid() ? m_dev->ftkbioPhysicalBlockSize() : FTKBIOERROR;
}

#if 0
fssize_t CBlockStream::BlockNumberGet(fssize_t pos)
{
	if ( pos >= m_size ) return 0; // early exit if the pos is past the end of the file

	int bs = m_dev->ftkbioBlockSize();

	fssize_t apos = pos+m_initialoffset;  // adjusted pos
	return blocknumxlat(apos / bs);	// map the block num from local sequential blocks into the device blocks
}
#endif

bool CBlockStream::blocknumxlat(INT64 &physicalblocknum, INT64 logicalblocknum) const
{
	if ( logicalblocknum >= m_bc ) return false;

	int l=0, r = m_runs.size()-1;
	while ( l <= r )	// binary search for the blockindex
	{
		int m = (l+r)/2;
		fssize_t logicalend = m_runs[m].logicalstart + m_runs[m].count;

		if ( m_runs[m].logicalstart <= logicalblocknum && logicalblocknum < logicalend )
		{
			if ( m_runs[m].physicalstart == -1 )
				physicalblocknum = -1;	// sparse
			else
				physicalblocknum = m_runs[m].physicalstart+logicalblocknum-m_runs[m].logicalstart;
			return true;
		}

		if ( logicalblocknum < m_runs[m].logicalstart )
			r = m-1;
		else
			l = m+1;
	}
	return false;
}

};		// end namespace

