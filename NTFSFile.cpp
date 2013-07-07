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

#include "NTFSFile.h"

#include "MFTstructs.h"
#include "NTFScommon.h"
#include "NTFSattributestructs.h"
#include "BlockStream.h"
#include "RamStream.h"
#include "SelfDestruct.h"
#include "ADIOString.h"

#include <malloc.h>

#define NTFSFILENAMEINDEX L"$I30"

namespace AccessData
{
namespace NTFS
{
using AccessData::w2s;
void CNTFSFile::initfields()
{
	m_ntfs = NULL;
	m_mft = NULL;
	m_baserecnum = FTKBIOERROR;
	m_referencecount = 0;
	m_flags = 0;
	m_firstsector = -1;
	m_firstcluster = -1;
}

void CNTFSFile::clearfields()
{
	m_ntfs = NULL;
	m_mft = NULL;
	m_baserecnum = 0;
	m_referencecount = 0;
	m_flags = 0;
	m_firstsector = -1;
	m_firstcluster = -1;
}

void CNTFSFile::assignfields(const CNTFSFile &rhs)
{
	m_ntfs = rhs.m_ntfs;
	m_mft = rhs.m_mft;
	m_baserecnum = rhs.m_baserecnum;
	m_referencecount = rhs.m_referencecount;
	m_flags = rhs.m_flags;
	m_firstsector = rhs.m_firstsector;
	m_firstcluster = rhs.m_firstcluster;
}

CNTFSFile::CNTFSFile()
{
	initfields();
}

CNTFSFile::CNTFSFile(const CNTFSFile &rhs)
{
	initfields();
	assignfields(rhs);
}

CNTFSFile::~CNTFSFile()
{
	clearfields();
}

CNTFSFile& CNTFSFile::operator=(const CNTFSFile &rhs)
{
	assignfields(rhs);
	return *this;
}

bool CNTFSFile::isvalid() const
{
	return m_ntfs != NULL;
}

void CNTFSFile::clear()
{
	clearfields();
}

void CNTFSFile::assign(const CNTFSFile &rhs)
{
	clearfields();
	assignfields(rhs);
}

bool CNTFSFile::open(CNTFS *ntfs, CMFT *mft, MFT_RECNUM recnum, UINT16 attribnum, bool slack)
{
	clear();
	if ( !ntfs || !mft || !recnum.isvalid() ) return false;

	CSelfDestruct<CNTFSFile> selfdestruct(this);

	if ( !m_mftrec.open(ntfs, mft, recnum) ) return false;

	m_baserecnum = recnum;
	m_ntfs = ntfs;
	m_mft = mft;
	m_flags = m_mftrec.getbaserecord()->flags;

	if ( !setdefaultattrib(attribnum, slack) ) return false;

	MFT_RECNUM precnum;
	if ( !m_mftrec.getfilename(m_filename, m_filenamealiases, precnum) ) return false;

    if ( m_baserecnum.RecNum() == sfrRootDir )
    {
    	m_filename = L""; // special case the root directory so we don't get "." as the filename
        m_pufid = -1;
    } else	// if ( precnum.RecNum() != m_baserecnum.RecNum() )
    {
		m_pufid = m_ntfs->orphandirufid();
        UINT16 origseqnum = precnum.SeqNum();
		precnum = MFT_RECNUM(0, precnum.RecNum() );		// strip the seqnum off
        CMFTRecord prec;
        if (
        	precnum.RecNum() != m_baserecnum.RecNum() &&		// make sure we're not looping
            prec.open( m_ntfs, m_mft, precnum) &&				// does it open okay?
            ((prec.seqnum() == origseqnum) || (m_mftrec.isdeleted() && prec.isdeleted() && prec.isdirectory() && prec.seqnum() == origseqnum + 1))	// make sure the seq nums match or kinda match if its our deleted parent directory
        )
        {
            int pa = prec.findattribute(atINDEXROOT, NTFSFILENAMEINDEX, -1);
            if ( pa == -1 ) pa = prec.findattribute(atDATA, L"", -1);
            if ( pa != -1 ) m_pufid = ntfs2ufid(pa, precnum.RecNum(), false);
        }
    }

    selfdestruct.disarm();
	return true;
}

bool CNTFSFile::setdefaultattrib(UINT16 attribnum, bool slack)
{
	SMFTAttribute *fa = m_mftrec.getattribute(attribnum, 0);

	// if the attrib doesn't exist, or if its resident and slack was requested, or if there isn't any slack
	if ( !fa || (slack && (fa->isresident() || (fa->streamlength_logical() == fa->streamlength_physical()) ) ) )
		return false;

	m_attribnum = attribnum;
	m_slack = slack;

    m_forkname = fa->getname();
    m_isfork = ((fa->attributetype == atDATA) && (m_forkname.length() != 0)) || (fa->attributetype == atINDEXALLOCATION);

	return true;
}

void CNTFSFile::getdataattribnums(vector<UINT16> &dataattribs)
{
	dataattribs.clear();
    for(int fatype, attribnum = 0; (fatype = m_mftrec.getattributetype(attribnum)) != atEND; attribnum++ )
    {
        if ( fatype == atDATA ) dataattribs.push_back( attribnum );
    }
}

void CNTFSFile::getdirattribnums(vector<UINT16> &dirattribs)
{
	dirattribs.clear();
    for(int fatype, attribnum = 0; (fatype = m_mftrec.getattributetype(attribnum)) != atEND; attribnum++ )
    {
        if ( fatype == atINDEXROOT ) dirattribs.push_back( attribnum );
    }
}

bool CNTFSFile::getdirattribnums(UINT16 &ir, UINT16 &ia, UINT16 &bm)
{
	ir = m_mftrec.findattribute(atINDEXROOT, NTFSFILENAMEINDEX, -1);
    if ( ir == 0xffff ) return false;

    ia = m_mftrec.findattribute(atINDEXALLOCATION, NTFSFILENAMEINDEX, -1);
    bm = m_mftrec.findattribute(atBITMAP, NTFSFILENAMEINDEX, -1);
    return true;
}

CBlockStream* CNTFSFile::openstream(UINT16 attribnum, bool slack)
{
	if ( !isvalid() ) return NULL;
	CBlockStream *stream = m_mftrec.openattribute(attribnum, slack);
	return stream;
}

//
// Overriden CFile methods
//


CBlockStream* CNTFSFile::Open()
{
	CBlockStream *s = openstream(m_attribnum, m_slack);
	if ( s && m_firstsector == -1 )
	{
		m_firstsector = s->GetBlock(0, NULL);
		m_firstcluster = s->GetBlock(0, m_ntfs);
	}
    return s;
}

CBlockStream *CNTFSFile::OpenSlack()
{
	return NULL;
}

UFID_t CNTFSFile::GetUFID() const
{
	return ntfs2ufid(m_attribnum, m_baserecnum.RecNum(), m_slack);
}

UFID_t CNTFSFile::GetParentUFID() const
{
	return m_pufid;
}

wstring CNTFSFile::GetName() const
{
	vector<wstring> filenames;
    GetNames(filenames);
    return filenames.size() ? filenames[0] : wstring();
}

CPath CNTFSFile::GetPath() const
{
	vector<CPath> paths;
    GetPaths(paths);
    return paths.size() ? paths[0] : CPath();
}

void CNTFSFile::GetNames(vector<wstring> &filenames) const
{
	wstring fname = m_filename; if ( m_isfork) { fname += L":"; fname += m_forkname; }
    filenames.push_back( fname );

    for(int i = 0; i < m_filenamealiases.size(); i++)
    {
        fname = m_filenamealiases[i]; if ( m_isfork) { fname += L":"; fname += m_forkname; }
        filenames.push_back( fname );
    }
}

void CNTFSFile::GetPaths(vector<CPath> &paths) const
{
	paths.clear();

    UINT16 pattrib;
    UINT64 prec;
    bool pslack;
    ufid2ntfs(m_pufid, pattrib, prec, pslack);
    string parentdir = (m_baserecnum.RecNum() != sfrRootDir) ? w2s(m_ntfs->getfilename( prec )) : string("\\");
    wstring fname;

    CPath temp;
    temp = parentdir;
    fname = m_filename; if ( m_isfork) { fname += L":"; fname += m_forkname; }
    temp += w2s(fname);
    paths.push_back( temp );

    for(int i = 0; i < m_filenamealiases.size(); i++)
    {
    	temp = parentdir;
        fname = m_filenamealiases[i]; if ( m_isfork) { fname += L":"; fname += m_forkname; }
        temp += w2s(fname);
        paths.push_back( temp );
    }
}

EFileType CNTFSFile::GetFileType() const
{
	return IsFile() ? (m_slack ? ftSlack : ftFile) : ftDirectory;
}

INT64 CNTFSFile::Length() const
{
	const SMFTAttribute *fa = m_mftrec.getattribute(m_attribnum, 0);
    return fa ?  fa->streamlength_logical() : 0;
}

INT64 CNTFSFile::PhysicalLength() const
{
	const SMFTAttribute *fa = m_mftrec.getattribute(m_attribnum, 0);
    return fa ?  fa->streamlength_physical() : 0;
}

time_t CNTFSFile::CreateDate() const
{
	SStandardInfoAttrib *sia = (SStandardInfoAttrib*)m_mftrec.getresidentattribute(atSTANDARDINFORMATION, NULL, -1);
	return sia ? sia->getctime() : 0;
}

time_t CNTFSFile::ModifyDate() const
{
	SStandardInfoAttrib *sia = (SStandardInfoAttrib*)m_mftrec.getresidentattribute(atSTANDARDINFORMATION, NULL, -1);
	return sia ? sia->getmtime() : 0;
}

time_t CNTFSFile::AccessDate() const
{
	SStandardInfoAttrib *sia = (SStandardInfoAttrib*)m_mftrec.getresidentattribute(atSTANDARDINFORMATION, NULL, -1);
	return sia ? sia->getatime() : 0;
}

UINT32 CNTFSFile::Flags() const
{
	UINT32 f = 0;

    f |= (m_mftrec.getattributetype(m_attribnum) != atINDEXROOT) ? FF_FILE : 0;
    f |= ((m_flags & 1) == 0) ? FF_DELETED : 0;
    f |= (m_mftrec.getattributetype(m_attribnum) == atINDEXROOT ) ? FF_DIRECTORY : 0; /*&& m_mftrec.getattributename(m_attribnum) == L"$I30"*/

	const SMFTAttribute *fa = m_mftrec.getattribute(m_attribnum, 0);
    if ( fa )
    {
	    f |= fa->iscompressed() ? DF_COMPRESSED : 0;
        f |= fa->isencrypted() ? DF_ENCRYPTED : 0;
    }
	SStandardInfoAttrib *sia = (SStandardInfoAttrib*)m_mftrec.getresidentattribute(atSTANDARDINFORMATION, NULL, -1);
    if ( sia )
    {
    	f |= sia->isreadonly() ? DF_READONLY : 0;
        f |= sia->ishidden() ? DF_HIDDEN : 0;
		f |= sia->issystem() ? DF_SYSTEM : 0;
        f |= sia->isarchive() ? DF_ARCHIVE : 0;
    }
    return f;
}

#if 0
bool CNTFSFile::ftkMetaDataListPopulate(CFTKMetaDataList &mdlist)
{
	if ( !isvalid() ) return false;

	bool isdatafork = false;

	wstring forkname;
	int parentattrib = -1;
	MFT_RECNUM parentrec;
	UFID_t pufid = -1;

	// Add some info about the default attribute
	const SMFTAttribute *fa = m_mftrec.getattribute(m_attribnum, 0);
	if ( fa )
	{
		if ( m_firstsector == -1 )
		{
			CBlockStream *stream = Open();
			if ( stream )
			{
				m_firstsector = stream->GetBlock(0, NULL);
				m_firstcluster = stream->GetBlock(0, m_ntfs);
				delete stream;
			}
		}

		if ( m_firstsector != -1 )
		{
			mdlist.ftkmdAdd( CFTKMetaData(mdiFIRSTCLUSTER, m_firstcluster) );
			mdlist.ftkmdAdd( CFTKMetaData(mdiFIRSTSECTOR, m_firstsector) );
		}

		// Add the stream sizes
		if ( m_slack )
		{
			int bs = m_ntfs->ftkbioBlockSize();
			int slacksize = fa->streamlength_physical() - fa->streamlength_logical();
			if ( slacksize > bs ) bs = ((slacksize / bs)+1)*bs;

			mdlist.ftkmdAdd( CFTKMetaData(mdiSIZELOGICAL, slacksize) );
			mdlist.ftkmdAdd( CFTKMetaData(mdiSIZEPHYSICAL, bs) );
		} else
		{
			mdlist.ftkmdAdd( CFTKMetaData(mdiSIZELOGICAL, fa->streamlength_logical()) );
			mdlist.ftkmdAdd( CFTKMetaData(mdiSIZEPHYSICAL, fa->streamlength_physical()) );
		}

		// Get the fork name
		forkname = fa->getname();
		isdatafork = (fa->attributetype == atDATA) && (forkname.length() != 0);
	}

	// Figure out the file's filename
	// Won't need to do the parentdirname stuff when the database stuff is working
	wstring filename;
	vector<wstring> filenamealiases;
	if ( m_mftrec.getfilename(filename, filenamealiases, parentrec) )
	{
		wstring parentdirname;
		if ( m_baserecnum.RecNum() == sfrRootDir )
		{
			parentdirname = L"\\";	// special case the root directory so we don't get "." as the filename
			filename = L"";
		} else
		{
			parentdirname = m_ntfs->getfilename(parentrec);
		}

		if ( isdatafork )
		{
			parentdirname += filename;
			filename = forkname;
		}
		if ( m_slack )
		{
			//parentdirname += filename;
			//filename = L"--slack--";
		}
		else
		{
			for(unsigned int i=0; i < filenamealiases.size(); i++) mdlist.ftkmdAdd( CFTKMetaData(mdiFILENAMEALIAS, filenamealiases[i]) );
		}
		mdlist.ftkmdAdd( CFTKMetaData(mdiFILEPATHNAME, parentdirname, filename) );
	}

	// Add the unique file id
	mdlist.ftkmdAdd( CFTKMetaData(mdiUFID, ntfs2ufid(m_attribnum, m_baserecnum.RecNum(), m_slack) ) );

	// Add the parent ufid
	if ( m_slack )															// if its slack, its parent is the non-slack version of the file
	{
		pufid = ntfs2ufid(m_attribnum, m_baserecnum.RecNum(), false);
	}
	else if ( isdatafork )													// if its a datafork, its parent is the atDATA attrib with name == "" or the atINDEXROOT attrib
	{
		parentattrib = m_mftrec.findattribute(atDATA, L"", -1);
		if ( parentattrib == -1 ) parentattrib = m_mftrec.findattribute(atINDEXROOT, NULL, -1);
	} else																	// else its a normal file... its parent is the parent directory's atINDEXROOT attrib
	{
		if ( m_baserecnum.RecNum() == sfrRootDir )
		{
			pufid = -1;
		} else
		{
			CMFTRecord parentmftrec;
			if ( parentmftrec.open(m_ntfs, m_mft, parentrec) )
			{
				parentattrib = parentmftrec.findattribute(atINDEXROOT, NULL, -1);
				if ( parentattrib != NULL )
				{
					pufid = ntfs2ufid(parentattrib, parentrec.RecNum(), false);
				}
			}
		}
	}
	mdlist.ftkmdAdd( CFTKMetaData(mdiPARENTUFID, pufid) );

	// Add the time info  ... maybe not for slack, but extensive testing at the Trevor Harrison NTFS testing academy has determined that
	// changing data in a fork changes the date/time stamp on the entire file, so we keep the time info for forks
	SStandardInfoAttrib *sia = (SStandardInfoAttrib*)m_mftrec.getresidentattribute(atSTANDARDINFORMATION, NULL, -1);
	if ( sia )
	{
		mdlist.ftkmdAdd( CFTKMetaData(mdiDATECREATE, sia->getctime()) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDATEMODIFY, sia->getmtime()) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDATEACCESS, sia->getatime()) );

		mdlist.ftkmdAdd( CFTKMetaData(mdiDFREADONLY, sia->isreadonly()) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDFHIDDEN, sia->ishidden()) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDFSYSTEM, sia->issystem()) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDFARCHIVE, sia->isarchive()) );

		// not using sia for these attribs because they might be different than the main file's attribs
		mdlist.ftkmdAdd( CFTKMetaData(mdiDFCOMPRESSED, fa->iscompressed() /*sia->iscompressed()*/) );
		mdlist.ftkmdAdd( CFTKMetaData(mdiDFENCRYPTED, fa->isencrypted() ));
	}

	// Add some flags
	mdlist.ftkmdAdd( CFTKMetaData(mdiISFILE, IsFile()) );
	mdlist.ftkmdAdd( CFTKMetaData(mdiISDIRECTORY, IsDirectory()) );
	mdlist.ftkmdAdd( CFTKMetaData(mdiISDELETED, IsDeleted()) );
	mdlist.ftkmdAdd( CFTKMetaData(mdiFILETYPE, GetFileType()) );

	return true;
}
#endif

}		// end namespace NTFS
}		// end namespace AccessData

