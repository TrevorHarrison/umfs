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

#ifndef NTFS_H
#define NTFS_H

#include "ADIOtypes.h"
#include "ADIOFSBase.h"
#include "MFT_RECNUM.h"
#include "MFT.h"
#include "NTFSBitmap.h"
#include "NTFSCommon.h"

namespace AccessData
{
namespace NTFS
{

// fwd defines
class CNTFSFile;
class CFTKNTFSDirectory;

class CNTFS : public CFSBase
{
public:
	CNTFS();
	~CNTFS();

	void				clear();

	fssize_t			getallocationrun(fssize_t startcluster, fssize_t maxrun, bool &b) { return m_bitmap.getrun(startcluster, maxrun, b); }
	UFID_t				getufidbypath(const CPath &path);
    CMFT&				getmft() { return m_mft; }
    CFTKBlockDevice*	getdev() { return m_dev; }

	//
	// CFTKFileSystem inherited functions
	//
    CFile*				OpenFile(UFID_t ufid);
	CDirectory*			GetDirectory(UFID_t ufid);
	CDirectory*			GetRootDirectory();
	UFID_t				GetRootDirectoryUFID() const;
	UFID_t				GetUnallocUFID() const;
	UFID_t				GetSlackUFID() const;
    bool				QueryUFIDs(vector<UFID_t> &ufids, int queryoptions);
	bool				IsBlockAllocated(fssize_t blocknum);
	bool				IsBlockUnallocated(fssize_t blocknum);
	fssize_t			FreeBlockCount();
	fssize_t			UsedBlockCount();
	EFileSystemType		FileSystemType() const;

	bool				Detect(CFTKBlockDevice *dev);
	CFileSystem*		Clone();
	bool				Mount(CFTKBlockDevice *dev, const CPath &indexdir, CJobCallBack &callback);
	//
	// end of CFTKFileSystem inherited functions
	//

	bool				ftkMetaDataListPopulate(CFTKMetaDataList &mdlist);
protected:
	friend class CNTFSDirectory;
	friend class CNTFSFile;

	CNTFSFile*		openfile(MFT_RECNUM recnum, UINT16 attribnum, bool slack);	// Open an ntfs file by record number
	wstring			getfilename(MFT_RECNUM fileref);	// get a file's name from cache	NEEDS WORK
    UFID_t			orphandirufid() { return ntfs2ufid(0, ORPHANRECNUM, false); }

	enum { UNALLOCRECNUM = -2, FSSLACKRECNUM = -3, ORPHANRECNUM = -4 };

	CMFT				m_mft;
	CNTFSBitmap			m_bitmap;
	UFID_t				m_rootdirufid;
	UINT32				m_volumeserialnumber;
	fssize_t			m_allocatedclusters;
private:
	void initfields();
	void clearfields();
	//void assignfields(const CNTFS &rhs);
	typedef CFileSystem inherited;
	CNTFS(const CNTFS &rhs);	// disallow
    CNTFS &operator=(const CNTFS &rhs);		// disallow
};

}		// end namespace NTFS
}		// end namespace AccessData

#endif