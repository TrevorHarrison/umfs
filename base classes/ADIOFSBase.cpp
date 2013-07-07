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

#include "ADIOFSBase.h"

#include "ADIOFile.h"
#include "Logger.h"

namespace AccessData
{

void CFSBase::initfields()
{
	m_dev = NULL;
	m_blockcount = 0;
	m_blocksize = 0;
	m_firstcluster = 0;
	m_cluster0block = 0;
	m_clustercount = 0;
	m_clustersize = 0;
	m_clusterscale = 0;
}

void CFSBase::clearfields()
{
	m_dev = NULL;
	m_blockcount = 0;
	m_blocksize = 0;
	m_firstcluster = 0;
	m_cluster0block = 0;
	m_clustercount = 0;
	m_clustersize = 0;
	m_clusterscale = 0;
}

#if 0
void CFSBase::assignfields(const CFSBase &rhs)
{
	m_dev = rhs.m_dev;
	m_blockcount = rhs.m_blockcount;
	m_blocksize = rhs.m_blocksize;
	m_firstcluster = rhs.m_firstcluster;
	m_cluster0block = rhs.m_cluster0block;
	m_clustercount = rhs.m_clustercount;
	m_clustersize = rhs.m_clustersize;
	m_clusterscale = rhs.m_clusterscale;
}
#endif

CFSBase::CFSBase()
{
	initfields();
}

#if 0
CFSBase::CFSBase(const CFSBase &rhs) : inherited(rhs)
{
	initfields();
	assignfields(rhs);
}
#endif

CFSBase::~CFSBase()
{
	clearfields();
}

bool CFSBase::isvalid() const
{
	return m_dev != NULL;
}

void CFSBase::clear()
{
	inherited::clear();
	clearfields();
}

#if 0
void CFSBase::assign(const CFSBase &rhs)
{
	inherited::assign(rhs);
	clearfields();
	assignfields(rhs);
}
#endif

CFile *CFSBase::OpenFile(UFID_t ufid)
{
	return NULL;
}

CDirectory *CFSBase::GetRootDirectory()
{
	return NULL;
}

CDirectory *CFSBase::GetDirectory(UFID_t ufid)
{
	return NULL;
}

UFID_t CFSBase::GetRootDirectoryUFID() const
{
	return -1;
}

UFID_t CFSBase::GetUnallocUFID() const
{
	return -1;
}

UFID_t CFSBase::GetSlackUFID() const
{
	return -1;
}

bool CFSBase::QueryUFIDs(vector<UFID_t> &ufids, int queryoptions)
{
	TRACEFUNC("CFSBase::QueryUFIDs");
	TRACEARG( queryoptions, "%x");

	TRACELOG1("returned %d ufids", ufids.size() );
    return true;
}

UFID_t CFSBase::GetParentUFID(UFID_t ufid)
{
    CFile *f = OpenFile(ufid);
    if ( !f ) return -1;
    UFID_t pufid = f->GetParentUFID();
    delete f;
    return pufid;
}

EFileType CFSBase::GetFileType(UFID_t ufid)
{
    CFile *f = OpenFile(ufid);
    if ( !f ) return ftUnknown;
    EFileType ft = f->GetFileType();
    delete f;
    return ft;
}

bool CFSBase::IsBlockAllocated(fssize_t blocknum)
{
	return false;
}

bool CFSBase::IsBlockUnallocated(fssize_t blocknum)
{
	return true;
}

fssize_t CFSBase::FreeBlockCount()
{
	return isvalid() ? m_clustersize * m_clustercount : -1;
}

fssize_t CFSBase::UsedBlockCount()
{
	return isvalid() ? 0 : -1;
}

bool CFSBase::ftkbioBlockRead(void *dest, fssize_t blocknum, int startoffset, int bytestoread)
{
	if ( bytestoread < 0 ) bytestoread = m_clustersize;
	if ( !isvalid() || !dest || (blocknum < m_firstcluster) || blocknum >= m_clustercount || (startoffset+bytestoread > m_clustersize) ) return false;

	if ( m_clusterscale == 1 ) return m_dev->ftkbioBlockRead(dest, m_cluster0block+blocknum, startoffset, bytestoread);

	fssize_t clusterstart = translateclusternum(blocknum); // m_cluster0block + (blocknum*m_clusterscale);

	char *cdest = (char *)dest;
	int startblock = startoffset / m_blocksize;
	int startblockoffset = startoffset % m_blocksize;
	int endblock = (startoffset+bytestoread) / m_blocksize;
	int endblockoffset = (startoffset+bytestoread) % m_blocksize;
	for(int i=startblock; i <= endblock; i++)
	{
		int so = i == startblock ? startblockoffset : 0;
		int len = i == endblock ? endblockoffset - so : m_blocksize - so;
		if ( !m_dev->ftkbioBlockRead(cdest, clusterstart+i, so, len) ) return false;
		cdest += len;
	}
	return true;
}

int CFSBase::ftkbioBlockReadN(void *dest, fssize_t startblocknum, int count)
{
	if ( !isvalid() || !dest || startblocknum < m_firstcluster ) return 0;

	fssize_t x = translateclusternum(startblocknum); // m_cluster0block+(startblocknum*m_clusterscale);
	int y = count * m_clusterscale;
	return m_dev->ftkbioBlockReadN(dest, x, y) / m_clusterscale;
}

int CFSBase::ftkbioBlockSize() const
{
	return m_clustersize;
}

fssize_t CFSBase::ftkbioFirstBlockNum() const
{
	return m_firstcluster;
}

fssize_t CFSBase::ftkbioBlockCount() const
{
	return m_clustercount;
}

int CFSBase::ftkbioPhysicalBlockSize() const
{
	return isvalid() ? m_dev->ftkbioPhysicalBlockSize() : -1;
}

bool CFSBase::ftkbioIsPhysicalDevice() const
{
	return false;
}

fssize_t CFSBase::ftkbioBlockNumTranslate(fssize_t blocknum, CFTKBlockDevice *dev) const
{
	if ( !isvalid() ) return -1;
	if ( dev == (CFTKBlockDevice*)this ) return blocknum;

	return m_dev->ftkbioBlockNumTranslate( translateclusternum(blocknum), dev);		// m_cluster0block + (blocknum*m_clusterscale)
}

CFTKBlockDevice::EVerifyResult CFSBase::ftkbioVerify(CJobCallBack &callback)
{
	return VERIFY_NOT_SUPPORTED;
}

bool CFSBase::ftkbioVerifySupported()
{
	return false;
}

void CFSBase::ftkbioFlush()
{
	if ( m_dev ) m_dev->ftkbioFlush();
}

bool CFSBase::ftkMetaDataListPopulate(CFTKMetaDataList &mdlist)
{
	return true;
}


bool CFSBase::setblockdevice(CFTKBlockDevice *dev)
{
	if ( !dev || !dev->isvalid() ) return false;
	m_dev = dev;
	m_blockcount = m_dev->ftkbioBlockCount();
	m_blocksize = m_dev->ftkbioBlockSize();
	m_firstcluster = 0;
	m_cluster0block = 0;
	m_clusterscale = 1;
	m_clustercount = m_blockcount;
	m_clustersize = m_blocksize;

	return true;
}

bool CFSBase::setclustertranslation(int scale, fssize_t cluster0block, fssize_t clustercount, fssize_t firstcluster)
{
	if ( scale == 0 ) return false;	// error, can't have a 0 scale
	m_clusterscale = scale;
	m_firstcluster = firstcluster;
	m_cluster0block = cluster0block;
	m_clustercount = clustercount;
	m_clustersize = m_blocksize * m_clusterscale;
	return true;
}

};		// end namespace

