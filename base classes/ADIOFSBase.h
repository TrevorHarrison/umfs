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

#ifndef ADIOFSBASE_H
#define ADIOFSBASE_H

#include "ADIOFileSystem.h"

namespace AccessData
{

// CFTKFSBase
class CFSBase : public CFileSystem
{
public:
	CFSBase();
	~CFSBase();

	bool			isvalid() const;
	void			clear();
	//void			assign(const CFSBase &rhs);

	//
	// CFTKFileSystem methods
	//
	CFile*				OpenFile(UFID_t ufid);
	CDirectory*			GetRootDirectory();
	CDirectory*			GetDirectory(UFID_t ufid);
	UFID_t				GetRootDirectoryUFID() const;
	UFID_t				GetUnallocUFID() const;
	UFID_t				GetSlackUFID() const;

	bool				QueryUFIDs(vector<UFID_t> &ufids, int queryoptions);
    UFID_t				GetParentUFID(UFID_t ufid);
    EFileType			GetFileType(UFID_t ufid);
	bool				IsBlockAllocated(fssize_t blocknum);
	bool				IsBlockUnallocated(fssize_t blocknum);
	INT64				FreeBlockCount();
	INT64				UsedBlockCount();

	//bool Detect(CFTKBlockDevice *dev);
	//CFTKFileSystem *Clone(CFTKBlockDevice *dev);
	//bool Mount(CFTKBlockDevice *dev);

	//
	// CFTKBlockDevice methods
	//
	bool			ftkbioBlockRead(void *dest, fssize_t blocknum, int startoffset=0, int bytestoread=-1);
	int				ftkbioBlockReadN(void *dest, fssize_t startblocknum, int count);
	int				ftkbioBlockSize() const;
	int				ftkbioPhysicalBlockSize() const;
	fssize_t		ftkbioFirstBlockNum() const;
	fssize_t		ftkbioBlockCount() const;
	bool			ftkbioIsPhysicalDevice() const;
	fssize_t		ftkbioBlockNumTranslate(fssize_t blocknum, CFTKBlockDevice *dev) const;
	EVerifyResult	ftkbioVerify(CJobCallBack &callback);
	bool			ftkbioVerifySupported();
    void			ftkbioFlush();

	bool			ftkMetaDataListPopulate(CFTKMetaDataList &mdlist);

protected:
	CFTKBlockDevice*	m_dev;				// the blockdevice the file system lives on

	fssize_t			m_blockcount;		// the number of m_dev blocks
	int					m_blocksize;		// the size of m_dev blocks (in bytes)

											// clusters are groups of m_dev blocks, usually a multiple of a power of 2, ie, 1, 2, 4, 8, 16, 32, 64
	fssize_t			m_firstcluster;		// the first valid cluster number that can be read
	fssize_t			m_cluster0block;	// the blocknum where cluster 0 starts.
	fssize_t			m_clustercount;		// the number of clusters
	int					m_clustersize;		// the size of a cluster (in bytes)
	int					m_clusterscale;		// how many m_dev blocks per cluster

	bool				setblockdevice(CFTKBlockDevice *dev);
	bool				setclustertranslation(int scale, fssize_t cluster0block, fssize_t clustercount, fssize_t m_firstcluster);
	fssize_t			translateclusternum(fssize_t clusternum) const { return m_cluster0block + (clusternum*m_clusterscale); }

private:
	void initfields();
	void clearfields();
	void assignfields(const CFSBase &rhs);

	typedef CFileSystem inherited;
	CFSBase(const CFSBase &rhs);		// disallow
	CFSBase &operator=(const CFSBase &rhs);		// disallow
};

};		// end namespace

#endif
