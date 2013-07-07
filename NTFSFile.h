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

#ifndef FTKNTFSFILE_H
#define FTKNTFSFILE_H

#include "ADIOFile.h"
#include "ADIOtypes.h"
#include "ADStream.h"
#include "NTFS.h"
#include "MFT.h"
#include "MFT_RECNUM.h"

namespace AccessData
{
namespace NTFS
{

// CNTFSFile
// This is a file on an NTFS volume.
// Natively, NTFS files can be uniquely specified by their record number in the MFT table.  (SFTKNTFSFileRef)
// The upper 2 bytes are reserved for a sequence number to prevent file refs from pointing to a re-used file record, and the lower
// 6 bytes are the record number.
// To map the NTFS unique file specification into the UFID_t type, which must specify the fork we want, we will put the
// fork's attribute index into the upper 2 bytes of the UFID_t and discard the sequence number info.
class CNTFSFile : public CFile
{
public:
	CNTFSFile();
	CNTFSFile(const CNTFSFile &rhs);
	~CNTFSFile();
    CNTFSFile &operator=(const CNTFSFile &rhs);

	bool			isvalid() const;
	void			clear();
	void			assign(const CNTFSFile &rhs);
	bool			open(CNTFS *ntfs, CMFT *mft, MFT_RECNUM recnum, UINT16 attribnum, bool slack);
	bool			setdefaultattrib(UINT16 attribnum, bool slack);
    void			getdataattribnums(vector<UINT16> &dataattribs);
    void			getdirattribnums(vector<UINT16> &dirattribs);
    bool			getdirattribnums(UINT16 &ir, UINT16 &ia, UINT16 &bm);
    CBlockStream*	openstream(UINT16 attribnum, bool slack);

	//
	// inherited CFile methods
	//
	CBlockStream*	Open();
	CBlockStream*	OpenSlack();
	UFID_t			GetUFID() const;
    UFID_t			GetParentUFID() const;
    wstring			GetName() const;
    CPath			GetPath() const;
    void			GetNames(vector<wstring> &filenames) const;
    void			GetPaths(vector<CPath> &paths) const;
	EFileType		GetFileType() const;
    INT64			Length() const;
    INT64			PhysicalLength() const;
    time_t			CreateDate() const;
    time_t			ModifyDate() const;
    time_t			AccessDate() const;
    UINT32			Flags() const;
	//
	// end of CFile methods
	//
protected:
	void		initfields();
	void		clearfields();
	void		assignfields(const CNTFSFile &rhs);
	void		mergedeletedrun(fssize_t blocknum, fssize_t streamrunlen, CBlockStream *stream);

	CNTFS*				m_ntfs;
	CMFT*				m_mft;
	MFT_RECNUM			m_baserecnum;
    UFID_t				m_pufid;
	CMFTRecord			m_mftrec;
	int					m_referencecount;
	int					m_flags;
	UINT16				m_attribnum;
	bool				m_slack;
	fssize_t			m_firstcluster;
	fssize_t			m_firstsector;

    wstring				m_filename;
    vector<wstring>		m_filenamealiases;
    bool				m_isfork;
    wstring				m_forkname;
private:
	typedef CFile inherited;
};

}		// end namespace NTFS
}		// end namespace AccessData

#endif