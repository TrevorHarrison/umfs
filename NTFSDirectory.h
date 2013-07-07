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

#ifndef NTFSDIRECTORY_H
#define NTFSDIRECTORY_H

#include "ADIOtypes.h"
#include "ADIODirectory.h"
#include "ADStream.h"
#include "NTFSFile.h"
#include "MFT_RECNUM.h"
#include "NTFSindexstructs.h"

#include <vector>

namespace AccessData
{
namespace NTFS
{

using std::vector;


class CNTFSDirectory : public CDirectory
{
public:
	CNTFSDirectory();
	CNTFSDirectory(const CNTFSDirectory &rhs);
	~CNTFSDirectory();

	bool	isvalid() const;
	void	clear();
	void	assign(const CNTFSDirectory &rhs);

	bool	open(CNTFS *ntfs, MFT_RECNUM fileref /*CFTKNTFSFile *afile*/);
	bool	read(CNTFS *ntfs, CStream *indexnodestream, NTFSindexentrylist *indexentrylist, int indexnodesize, int blocksize, const wstring &name, int attribs);

    int		getslackspace();

	//
	// inherited CFTKDirectory methods
	//
	CFile*	FindFirst(const wstring &name, int attribs);
	CFile*	FindNext(const wstring &name, int attribs);
	void	FindReset();
protected:

	CNTFS*					m_ntfs;
    CNTFSFile				m_file;
	MFT_RECNUM				m_mftrecnum;
    UINT16					m_ir_attribnum;
    UINT16					m_ia_attribnum;
    UINT16					m_bm_attribnum;
	vector< UFID_t >		m_ufidlist;
	unsigned int			m_currententry;
private:
	void initfields();
	void clearfields();
	void assignfields(const CNTFSDirectory &rhs);

	typedef CDirectory inherited;
};

}		// end namespace NTFS
}		// end namespace AccessData


#endif