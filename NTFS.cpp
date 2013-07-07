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

#include "NTFS.h"
#include "MFTstructs.h"
#include "ADIOString.h"
#include "BlockStream.h"
#include "RamStream.h"
#include "NTFSFile.h"
#include "NTFSDirectory.h"
#include "ADIOFileGeneric.h"
#include "Logger.h"
#include "SelfDestruct.h"

#include <malloc.h>
namespace AccessData
{
namespace NTFS
{
using AccessData::s2w;		// hack to workaround BCC bug

static void foo()
{
	new CNTFS;
}

void CNTFS::initfields()
{
	m_mft.clear();
	m_bitmap.clear();
	m_rootdirufid = -1;
	m_volumeserialnumber = 0;
	m_allocatedclusters = 0;
}

void CNTFS::clearfields()
{
	m_mft.clear();
	m_bitmap.clear();
	m_rootdirufid = -1;
	m_volumeserialnumber = 0;
	m_allocatedclusters = 0;
}

/*void CNTFS::assignfields(const CNTFS &rhs)
{
	m_dev = rhs.m_dev;
	m_devblocksize = rhs.m_devblocksize;
	m_devblockcount = rhs.m_devblockcount;
	m_blockscale = rhs.m_blockscale;
	m_blockcount = rhs.m_blockcount;
	//m_mft.assign(rhs.m_mft); bad.... would copy rhs.mft's parent *fs pointer.... bad, bad.
}*/

CNTFS::CNTFS()
{
	initfields();
}

CNTFS::~CNTFS()
{
	clearfields();
}

void CNTFS::clear()
{
	//CSingleLock lock(&m_Mutex);
	//lock.Lock();

	inherited::clear();
	clearfields();
}

UFID_t CNTFS::getufidbypath(const CPath &path)
{
	if ( !path.is(PATH_WIN) || path.is(PATH_UNC) ) return -1;

    vector<string> pathparts;
    path.explode(pathparts);

	CDirectory *dir = GetRootDirectory();
    bool error = false;
    for(unsigned int i = 0; i < pathparts.size() - 1 && dir; i++)
    {
    	CFile *f = dir->FindFirst( s2w(pathparts[i]), dsaDIRECTORY );
        if ( !f ) { error = true; break; }

        delete dir;

        dir = GetDirectory( f->GetUFID() );
        delete f;
    }
    if ( error || !dir ) return -1;

    CFile *f = dir->FindFirst( s2w(pathparts[ pathparts.size() - 1 ]), dsaFILE );
    delete dir;
    if ( !f ) return -1;
    UFID_t result = f->GetUFID();
    delete f;
    return result;
}

CNTFSFile *CNTFS::openfile(MFT_RECNUM recnum, UINT16 attribnum, bool slack)
{
	if ( !isvalid() || !m_mft.isvalidrecnum(recnum) ) return NULL;

	if ( recnum.RecNum() == UNALLOCRECNUM || recnum.RecNum() == FSSLACKRECNUM ) return NULL;

	CNTFSFile *newfile = new CNTFSFile;
	if ( !newfile ) return NULL;
	if ( newfile->open(this, &m_mft, recnum, attribnum, slack) ) return newfile;

	delete newfile;
	return NULL;
}

wstring CNTFS::getfilename(MFT_RECNUM recnum)
{
	wstring path;
	while ( recnum.RecNum() != sfrRootDir )
	{
		CMFTRecord mftrec;
		if ( !mftrec.open(this, &m_mft, recnum) ) break;

		wstring filename;
		vector<wstring> filenamealiases;
		MFT_RECNUM parentrec;
		if ( !mftrec.getfilename(filename, filenamealiases, parentrec) ) break;
		recnum = parentrec;
		path = wstring(L"\\") + filename + path;
	}
	if ( path.length() == 0 ) path = L"\\";
	if ( recnum.RecNum() != sfrRootDir ) path = wstring(L"\\<orphan>") + path;
	return path;
}

//
// CFTKFileSystem inherited functions
//
CFile *CNTFS::OpenFile(UFID_t ufid)
{
	TRACEFUNC("CNTFS::OpenFile");


	if ( !isvalid() || ufid == -1 ) return NULL;

	UINT16 attribnum;
	UINT64 recnum;
	bool isslack;
	ufid2ntfs(ufid, attribnum, recnum, isslack);
	TRACEARG( recnum, "%I64d");
	TRACEARG( attribnum, "%d");
	TRACEARG( isslack, "%d");

	switch ( recnum )
	{
		case (UINT32)UNALLOCRECNUM:
		{
			CBlockStream *newstream = new CBlockStream;
			if ( !newstream ) return NULL;

			newstream->SetDev(this);

			bool b;
			fssize_t start=0, len;
			while ( (len = m_bitmap.getrun(start, -1, b)) != 0 )
			{
				if ( b == false )	// unallocated
				{
					newstream->AddRun(start, len);
				}
				start += len;
			}
			newstream->SetLength( (fssize_t)newstream->BlockCount() * m_clustersize );

			CGenericFile *newfile = new CGenericFile(newstream, ufid, -1, ftUnalloc);
			if ( !newfile ) return NULL;
			if ( !newfile->isvalid() ) { delete newfile; return NULL; }

			return newfile;
		}
		break;
		case (UINT32)FSSLACKRECNUM:
		{
			return NULL;
		}
		break;
        case (UINT32)ORPHANRECNUM:
        {
        	CRamBlockStream *stream = new CRamBlockStream;
            stream->SetDev(this);
            stream->setbuffer("", 1);
        	CGenericFile *newfile = new CGenericFile( stream, ufid, -1, ftDirectory, CPath("Orphan"));
            return newfile;
        }
        break;
		default:
		{
			return openfile( MFT_RECNUM(recnum), attribnum, isslack);
		}
		break;
	}
	return NULL;
}

CDirectory *CNTFS::GetDirectory(UFID_t ufid)
{
	//CSingleLock lock(&m_Mutex);
	//lock.Lock();

	UINT16 attribnum;
    UINT64 recnum;
    bool isslack;
    ufid2ntfs(ufid, attribnum, recnum, isslack);

	CNTFSDirectory *dir = new CNTFSDirectory;
	if ( !dir ) return NULL;
	if ( !dir->open(this, MFT_RECNUM( recnum ) ) ) { delete dir; return NULL; }
	return dir;
}

CDirectory *CNTFS::GetRootDirectory()
{
	//CSingleLock lock(&m_Mutex);
	//lock.Lock();

	return GetDirectory( ntfs2ufid(0, sfrRootDir, false) );
}

UFID_t CNTFS::GetRootDirectoryUFID() const
{
	//CSingleLock lock((CMutex*)&m_Mutex);
	//lock.Lock();

	return m_rootdirufid;
}

UFID_t CNTFS::GetUnallocUFID() const
{
	//CSingleLock lock((CMutex*)&m_Mutex);
	//lock.Lock();

	return ntfs2ufid(0, UNALLOCRECNUM, false);
}

UFID_t CNTFS::GetSlackUFID() const
{
	//CSingleLock lock((CMutex*)&m_Mutex);
	//lock.Lock();

	return ntfs2ufid(0, FSSLACKRECNUM, false);
}

bool CNTFS::QueryUFIDs(vector<UFID_t> &ufids, int queryoptions)
{
    if ( (queryoptions & (INCLUDEFILES|INCLUDEDIRS) ) != 0 )
    {
    	bool getslack = (queryoptions & INCLUDESLACK) == INCLUDESLACK;
        for( int recnum = 0, reccount = m_mft.recordcount(); recnum < reccount; recnum++ )
        {
            if ( recnum == sfrBadClusters ) continue;

            CNTFSFile ntfsfile;
            if ( !ntfsfile.open(this, &m_mft, recnum, 0, false) ) continue;

			UINT16 ir, ar, bm;
			bool hasdir = ntfsfile.getdirattribnums(ir, ar, bm);

            if ( (queryoptions & INCLUDEFILES) == INCLUDEFILES )
            {
	            vector<UINT16> attribnums;
    	        ntfsfile.getdataattribnums( attribnums );
                for(int i = 0; i < attribnums.size(); i++)
                {
                	UINT16 attribnum = attribnums[i];
                    ufids.push_back( ntfs2ufid(attribnum, recnum, false) );
                    if ( getslack && ntfsfile.setdefaultattrib(attribnum, true) ) ufids.push_back( ntfs2ufid(attribnum, recnum, true) );
                }
                if ( hasdir && ar != 0xFFFF )
                	ufids.push_back( ntfs2ufid(ar, recnum, false) );
            }
            if ( hasdir && (queryoptions & INCLUDEDIRS) == INCLUDEDIRS )
            {
				ufids.push_back( ntfs2ufid(ir, recnum, false) );
            }
        }
    }
    if ( (queryoptions & INCLUDESPECIAL) == INCLUDESPECIAL )
    {
		ufids.push_back( ntfs2ufid(0, UNALLOCRECNUM, false) );
        if ( translateclusternum(m_clustercount) < m_blockcount ) ufids.push_back( ntfs2ufid(0, FSSLACKRECNUM, false) );
    }
    if ( (queryoptions & INCLUDEDIRS) == INCLUDEDIRS )
    {
		ufids.push_back( ntfs2ufid(0, ORPHANRECNUM, false) );
    }
//#if 0
    FILE *fout = fopen("c:\\query.txt", "at");
    if ( fout )
    {
    	LARGE_INTEGER li;
    	for(int i = 0; i < ufids.size(); i++)
        {
        	UINT16 attrib;
            UINT64 recnum;
            bool slack;
            ufid2ntfs(ufids[i], attrib, recnum, slack);
			li.QuadPart = ufids[i];
        	fprintf(fout, "%X%X", li.HighPart, li.LowPart);
            li.QuadPart = recnum;
            fprintf(fout, ": recnum=%X%X, attrib=%d, slack=%d\n", li.HighPart, li.LowPart, attrib, slack);
        }
    	fclose(fout);
    }
//#endif
    return true;
}

bool CNTFS::IsBlockAllocated(fssize_t blocknum)
{
	//CSingleLock lock(&m_Mutex);
	//lock.Lock();

	if ( !isvalid() || blocknum > m_blockcount ) return false;
	return m_bitmap.getbit(blocknum);
}

bool CNTFS::IsBlockUnallocated(fssize_t blocknum)
{
	//CSingleLock lock(&m_Mutex);
	//lock.Lock();

	if ( !isvalid() || blocknum > m_blockcount ) return false;
	return !m_bitmap.getbit(blocknum);
}

fssize_t CNTFS::FreeBlockCount()
{
	return isvalid() ? m_clustercount - m_allocatedclusters : -1;
}

fssize_t CNTFS::UsedBlockCount()
{
	return isvalid() ? m_allocatedclusters : -1;
}

EFileSystemType CNTFS::FileSystemType() const
{
	return fsNTFS;
}

bool CNTFS::Detect(CFTKBlockDevice *dev)
{
	if ( !dev ) return false;

	SBootRecord bootrec;
	return bootrec.Read(dev) && bootrec.isvalidntfs();
}

CFileSystem *CNTFS::Clone()
{
	return new CNTFS;
}

bool CNTFS::Mount(CFTKBlockDevice *dev, const CPath &indexdir, CJobCallBack &callback)
{
	CSelfDestruct<CNTFS> selfdestruct(this);

	clear();

	SBootRecord bootrec;
	if ( !dev || !bootrec.Read(dev) || !bootrec.isvalidntfs() ) { TRACELOG0("not valid bootrec"); return false; }

	// make sure that the BPB and the actual device's blocksize match
	int bpbscale = bootrec.bpb.clustersize;
	int devbs = dev->ftkbioBlockSize();
	int bpbbs = bootrec.bpb.blocksize;
	if ( bpbscale == 0 ) return false;
	if ( bpbbs != devbs ) return false;

	// This sets the block to cluster translation info.
	int bpbbc = bootrec.bpb.ntfs.blockcount;
	if ( !setblockdevice(dev) || !setclustertranslation(bpbscale, 0, bpbbc / bpbscale, 0) ) { TRACELOG0("failed to set bd and cluster xlat"); return false; }

	m_volumeserialnumber = bootrec.bpb.ntfs.volumeserialnumber;


	// Read the mft
	if ( !m_mft.bootstrap(this, &bootrec) ) { TRACELOG0("could not bootstrap mft"); return false; }

	// Read the bitmap that allocates volume clusters
	CMFTRecord mftrec;
	if ( !mftrec.open(this, &m_mft, MFT_RECNUM(sfrBitmap)) ) { TRACELOG0("could not open $bitmap"); return false; }

	CStream *s = mftrec.openattribute(atDATA, NULL, -1);
	if ( !s ) { TRACELOG0("could not get $bitmap stream"); return false; }

	if ( !m_bitmap.open(s) ) { delete s; TRACELOG0("could not init m_bitmap with stream"); return false; }

	// Count the allocated / unallocated sectors
	for(fssize_t b = 0; b < m_clustercount; /*nada*/)
	{
		bool allocd;
		fssize_t x = m_bitmap.getrun(b, m_clustercount-b, allocd);
		if ( x == 0 ) { TRACELOG0("io error reading volume bitmap"); return false; }
		if ( allocd ) m_allocatedclusters += x;
		b += x;
	}

	// Figure out the attrib num for the root directory
	if ( !mftrec.open(this, &m_mft, MFT_RECNUM(sfrRootDir)) ) { TRACELOG0("failed to open rootdir mft record"); return false; }
	int attribnum = mftrec.findattribute(atINDEXROOT, NULL, -1);
	if ( attribnum < 0 ) { TRACELOG0("failed to get attribnum for rootdir"); return false; }
	m_rootdirufid = ntfs2ufid(sfrRootDir, attribnum, false);

	selfdestruct.disarm();
	return true;
}

//
// end of CFTKFileSystem inherited functions
//


#define MAX_NTFSVOLUMENAMESTRING 25
bool CNTFS::ftkMetaDataListPopulate(CFTKMetaDataList &mdlist)
{
	if ( !isvalid() ) return false;

	CMFTRecord mftrec;
	if ( mftrec.open(this, &m_mft, MFT_RECNUM(sfrVolume)) )
	{
		int i = mftrec.findattribute(atVOLUMENAME, NULL, -1);
		if ( i >= 0 )
		{
			SMFTAttribute *fa = mftrec.getattribute(i, 0);
			wchar_t *volname = (wchar_t *)mftrec.getresidentattribute(i);

			if ( fa && volname ) 
			{
				mdlist.ftkmdAdd( CFTKMetaData(mdiVOLUMENAME, wz2w(volname, fa->r.streamlength / sizeof(wchar_t) )) );
			}
		}
	}
	mdlist.ftkmdAdd( CFTKMetaData(mdiVOLUMESERIALNUMBER, m_volumeserialnumber) );

	return true;
}

}		// end namespace NTFS
}		// end namespace AccessData

