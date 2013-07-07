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

#include "MFT.h"
#include "MFTstructs.h"
#include "NTFSattributestructs.h"
#include "NTFS.h"
#include "NTFSFile.h"
#include "NTFSCommon.h"
#include "RamStream.h"
#include "Logger.h"
#include "SelfDestruct.h"

#include <malloc.h>
#include <stdlib.h>
#include <assert.h>

namespace AccessData
{
namespace NTFS
{

void CMFT::initfields()
{
	m_ntfs = NULL;
	m_stream = NULL;
	m_physicalblocksize = 0;
	m_recsize = 0;
	m_reccount = 0;
    m_recoverhead = 0;
}

void CMFT::clearfields()
{
	m_ntfs = NULL;
	if ( m_stream ) delete m_stream;
	m_stream = NULL;
	m_physicalblocksize = 0;
	m_reccount = 0;
	m_recsize = 0;
    m_recoverhead = 0;
}

void CMFT::assignfields(const CMFT &rhs)
{
	m_ntfs = rhs.m_ntfs;
    m_stream = rhs.m_stream ? rhs.m_stream->BlockStreamDup() : NULL;
	m_physicalblocksize = rhs.m_physicalblocksize;
	m_recsize = rhs.m_recsize;
	m_reccount = rhs.m_reccount;
    m_recoverhead = rhs.m_recoverhead;
}

CMFT::CMFT()
{
	initfields();
}

CMFT::~CMFT()
{
	clearfields();
}

bool CMFT::isvalid() const
{
	return m_ntfs != NULL;
}

void CMFT::clear()
{
	clearfields();
}

#define MFT_RESERVEDFILERECS 16
#define MFT_MINRECSIZE 512
bool CMFT::open(CNTFS *ntfs, CBlockStream *mftstream, int recsize, int physicalblocksize)
{
	clear();
	if (
			!ntfs || !mftstream || !mftstream->isvalid() ||
			recsize < MFT_MINRECSIZE ||
			(mftstream->Length() / recsize) < MFT_RESERVEDFILERECS
			)
    {
    	TRACELOG0("bad params");
        return false;
    }

	m_ntfs = ntfs;
	m_stream = mftstream;
	m_physicalblocksize = physicalblocksize;
	m_recsize = recsize;
	m_reccount = m_stream->Length() / m_recsize;

	return true;
}

bool CMFT::bootstrap(CNTFS *ntfs, SBootRecord *bootrec)
{
	CSelfDestruct<CMFT> selfdestruct(this);

	clear();
	if ( !ntfs || !bootrec ) return false;

	// set ourselves to a what a minimal mft would occupy
	m_ntfs = ntfs;
	m_physicalblocksize = bootrec->bpb.blocksize;
	m_recsize = 0;
	if ( bootrec->bpb.ntfs.mftrecordsize > 0x80)
	{
		assert( bootrec->bpb.ntfs.mftrecordsize < 255 );
		m_recsize = 1 << (256 - bootrec->bpb.ntfs.mftrecordsize);
	} else
	{
		m_recsize = bootrec->bpb.blocksize * bootrec->bpb.clustersize * bootrec->bpb.ntfs.mftrecordsize;
	}
	m_reccount = MFT_RESERVEDFILERECS;

	// construct a stream by hand
	CBlockStream *tempstream = new CBlockStream;
	tempstream->SetDev( m_ntfs );
	tempstream->SetInitialOffset( 0 );
	tempstream->SetLength( m_recsize * MFT_RESERVEDFILERECS );
	tempstream->AddRun( bootrec->bpb.ntfs.mftstart, bootrec->bpb.clustersize * bootrec->bpb.ntfs.mftrecordsize * MFT_RESERVEDFILERECS);
	m_stream = tempstream;

	// now we represent a minimal mft.  we can ask for any record number < 16
	// that also means if we return from here, an isvalid() will show true, so do a clear before returning false

	// read the actual mft from our minimal mft
	CMFTRecord mftrec;
	if ( !mftrec.open(m_ntfs, this, MFT_RECNUM(sfrMFT)) ) { TRACELOG0("failed to rec $mft record"); return false; }

	CBlockStream *newmftstream = mftrec.openattribute(atDATA, L"", -1);
	if ( !newmftstream ) { TRACELOG0("failed to open mft attribute stream"); return false; }

	m_stream = newmftstream;
	delete tempstream;

	m_reccount = m_stream->Length() / m_recsize;

	SMFTRecord *temprec = readrawrecord(0);
	if ( temprec )
    {
        m_recoverhead = temprec->attributeoffset + 8;		// how much is used at the beginning of the record plus the 0xFFFF terminator
        free(temprec);
    }

	selfdestruct.disarm();
	return true;
}

CMFTRecord*	CMFT::readrecord(MFT_RECNUM recnum)
{
	if ( !isvalidrecnum(recnum) ) return NULL;

    CMFTRecord *rec = new CMFTRecord;
    if ( !rec ) return NULL;
    if ( !rec->open(m_ntfs, this, recnum) ) { delete rec; return NULL; }

    return rec;
}

SMFTRecord*	CMFT::readrawrecord(MFT_RECNUM recnum)
{
	if ( !isvalid() || !isvalidrecnum(recnum) ) return NULL;

    SMFTRecord *rec = (SMFTRecord*)malloc(m_recsize);
    if ( !rec ) return NULL;

    bool result = (m_stream->Read(rec, m_recsize, recnum.RecNum() * m_recsize) == m_recsize) &&
					rec->isvalid() &&
					rec->dofixup( m_physicalblocksize ) &&
					( recnum.isseqwildcard() ||  rec->sequencenumber == recnum.SeqNum() );
    if ( !result )
    {
    	free(rec);
        return NULL;
    }
    return rec;
}

int CMFT::freecount()
{
	CMFTRecord mftrec;
	if ( !mftrec.open(m_ntfs, this, MFT_RECNUM(sfrMFT)) ) return 0;

	CBlockStream *bms = mftrec.openattribute(atBITMAP, L"", -1);
    if ( !bms ) return 0;

    CNTFSBitmap bm;
    bm.open(bms);

    INT64 bits_set, bits_free;
    bm.summary(bits_set, bits_free);

    return bits_free;
}

int CMFT::alloccount()
{
	CMFTRecord mftrec;
	if ( !mftrec.open(m_ntfs, this, MFT_RECNUM(sfrMFT)) ) return 0;

	CBlockStream *bms = mftrec.openattribute(atBITMAP, L"", -1);
    if ( !bms ) return 0;

    CNTFSBitmap bm;
    bm.open(bms);

    INT64 bits_set, bits_free;
    bm.summary(bits_set, bits_free);

    return bits_set;
}

CNTFSBitmap* CMFT::getbitmap()
{
	CMFTRecord mftrec;
	if ( !mftrec.open(m_ntfs, this, MFT_RECNUM(sfrMFT)) ) { TRACELOG0("failed to rec $mft record"); return NULL; }

	CBlockStream *bms = mftrec.openattribute(atBITMAP, L"", -1);
	if ( !bms ) return NULL;

	CNTFSBitmap *bm = new CNTFSBitmap;
	bm->open(bms);

	return bm;
}

bool CMFT::getrecinfo(MFT_RECNUM recnum, INT64 &startcluster, int &length)
{
	startcluster = m_stream->GetBlock( recnum.RecNum() * m_recsize );
    length = div_roundup(m_recsize, m_stream->BlockSize());

    return true;	                         
}
//-----------------------------------------------------------------------------------

void CMFTRecord::AttribInfo::clear()
{
	attributetype = 0;
	name = L"";
	identifier = 0;
	streamlength_logical = 0;
	streamlength_physical = 0;
	fragments.clear();
}

bool CMFTRecord::AttribInfo::read(SMFTAttribute *fa, MFT_RECNUM recnum)
{
	if ( !fa ) return false;

	clear();

	attributetype = fa->attributetype;
	identifier = fa->identifier;
	name = fa->getname();

	fragments.push_back( AttribFragInfo(recnum, fa) );

	// is this stuff even used?
	if ( fa->isresident() )
	{
		streamlength_logical = streamlength_physical = fa->r.streamlength;
	} else
	{
		streamlength_logical = fa->nr.streamlength_real;
		streamlength_physical = fa->nr.streamlength_allocated;
	}

	return true;
}

bool CMFTRecord::AttribInfo::read(CStream *fin)
{
	if ( !fin || !fin->isvalid() ) return false;

	clear();

	bool isfirst = true;

	void *alrbuffer = alloca( ALArec::MAXRECORDLENGTH );
	if ( !alrbuffer ) return false;

	while ( true )
	{
		fssize_t pos = fin->Tell();

		ALArec *rec = ALArec::read(fin, alrbuffer, ALArec::MAXRECORDLENGTH);

		// if we hit the end of the stream, return success based on if we've read previous records
		if ( !rec ) return isfirst != true;

		if ( isfirst )
		{
			attributetype = rec->attributetype;
			identifier = rec->identifier;
			name = rec->getname();
			isfirst = false;
		} else
		{
			// if the new record doesn't match our first record, rewind the stream to its original position
			// and return
			if ( rec->attributetype != attributetype || rec->identifier != identifier || rec->getname() != name )
			{
				fin->Seek(pos, CStream::swSET);
				break; // return true;
			}
		}
        fragments.push_back( AttribFragInfo(rec->attributelocation, NULL) );
	}

	return true;
}

//-------------------------------------------------

void CMFTRecord::initfields()
{
	m_ntfs = NULL;
	m_mft = NULL;
	m_recsize = 0;
	m_baserec = NULL;
}

void CMFTRecord::clearfields()
{
	m_ntfs = NULL;
	m_mft = NULL;
	m_recsize = 0;
	m_baserec = NULL;

	for(unsigned int i=0; i < m_records.size(); i++)
	{
		if ( m_records[i].record ) free(m_records[i].record);
	}
	m_records.clear();
	m_attributes.clear();
}

CMFTRecord::CMFTRecord()
{
	initfields();
}

CMFTRecord::~CMFTRecord()
{
	clearfields();
}

bool CMFTRecord::isvalid() const
{
	return m_ntfs != NULL;
}

void CMFTRecord::clear()
{
	clearfields();
}

bool CMFTRecord::open(CNTFS *ntfs, CMFT *mft, MFT_RECNUM recnum)
{
	if ( !ntfs || !mft || !mft->isvalidrecnum(recnum) ) return false;
	clear();

    CSelfDestruct<CMFTRecord> selfdestruct(this);

	m_recsize = mft->recordsize();
	m_ntfs = ntfs;
	m_mft = mft;
	m_baserecnum = recnum;

	// Load the base record from the MFT
	m_baserec = m_mft->readrawrecord( recnum ); // (SMFTRecord*)malloc( m_recsize );
	if ( !m_baserec ) return false;
	m_records.push_back( SubRecordInfo(recnum, m_baserec) );	// push it on the vector early so the dtor will take care of cleaning up this pointer

    if ( !m_baserec->isbaserecord() ) return false;

	// Build a list of the attributes that are in the baserecord
	for(SMFTAttribute *fa = m_baserec->getfirstattribute(); fa && fa->attributetype != atEND; fa = m_baserec->getnextattribute(fa) )
	{
		AttribInfo ai;
		if ( !ai.read(fa, recnum) ) return false;
		m_attributes.push_back( ai );
	}

	// Using the attribute list, try to find the ALA and open it.
	// If it does exist, re-init the attribute list with the stream
	CBlockStream *alastream = openattribute(atATTRIBUTELIST, NULL, -1);
	if ( alastream )
	{
		m_attributes.clear();
		while ( !alastream->Eof() )
		{
			AttribInfo ai;
			if ( !ai.read(alastream) ) break;
			m_attributes.push_back(ai);
		}
		delete alastream;
		if ( m_attributes.size() == 0 ) return false;

        // Now fixup the attribute pointers by loading the subrecords
        for(unsigned int i = 0; i < m_attributes.size(); i++)
        {
        	AttribInfo &ai = m_attributes[i];
            for(unsigned int j = 0; j < ai.fragments.size(); j++)
            {
            	AttribFragInfo &afi = ai.fragments[j];
            	SMFTRecord *subrec = getsubrecord(afi.location);
                if ( !subrec ) return false;
                SMFTAttribute *fa = subrec->findattribute(ai.attributetype, ai.name.c_str(), ai.identifier, 0);
                if ( !fa ) return false;
                afi.data = fa;
            }
        }
	}

    selfdestruct.disarm();
	return true;
}

bool CMFTRecord::isdeleted() const
{
	return isvalid() ? !m_baserec->isinuse() : false;
}

bool CMFTRecord::isdirectory() const
{
	return findattribute(atINDEXROOT, NULL, -1) != -1;
}

UINT16 CMFTRecord::seqnum() const
{
	return m_baserec->sequencenumber;
}

CBlockStream *CMFTRecord::openattribute(int attributetype, const wchar_t *name, int identifier, bool slack)
{
	int i = findattribute(attributetype, name, identifier);
	return i >= 0 ? openattribute(i, slack) : NULL;
}

CBlockStream *CMFTRecord::openattribute(int index, bool slack)
{
	if ( !isvalid() || !isvalidattributenum(index) ) return NULL;

	AttribInfo &ai = m_attributes[index];

	bool deleted = isdeleted();
	CBlockStream *istream = NULL;
	fssize_t slacksize = 0;

	// Build the stream first
	for(int i=0, fragcount = ai.fragments.size(); i < fragcount; i++)
	{
		SMFTAttribute *fa = ai.fragments[i].data;
		if ( !fa ) return NULL;

		if ( fa->iscompressed() )
        {
        	return NULL;
        }

		if ( fa->isresident() )
		{
			if ( slack ) return NULL;	// can't open the slack on a resident file, dummy!
			if ( fragcount > 1 ) return NULL;

            INT64 startcluster;
            int length;
            if ( !m_mft->getrecinfo( ai.fragments[i].location, startcluster, length) ) return NULL;

			CRamBlockStream *ramstream = new CRamBlockStream;
			if ( !ramstream ) return NULL;

            ramstream->SetDev( m_ntfs );
            ramstream->AddRun( startcluster, length );
			ramstream->setbuffer( fa->residentstream(), fa->r.streamlength );
			return ramstream;
		}
		if ( i == 0 )
		{
			istream = new CBlockStream;
			if ( !istream ) return NULL;
			istream->SetDev( m_ntfs );
			istream->SetInitialOffset( 0 );
			istream->SetLength( fa->nr.streamlength_real );

			if ( slack ) slacksize = fa->nr.streamlength_allocated - fa->nr.streamlength_real;
		}

		const NTFSfileruns *runs = fa->getruns();
		if ( !runs ) return NULL;

		fssize_t prevoffset = 0;
		fssize_t offset = 0, length = 0;
		bool sparse;
		for(const char *cp = runs->getfirstrun(offset, length, sparse); cp; cp = runs->getnextrun(offset, length, sparse, cp) )
		{
			prevoffset += offset;

			if ( sparse )
			{
				istream->AddRun(-1, length);
			} else
			{
				if ( deleted )		// if the record is marked as deleted, we want to make sure we don't read allocated clusters from the disk
				{
					mergedeletedrun(prevoffset, length, istream);
				} else
				{
					istream->AddRun(prevoffset, length);
				}
			}
		}
	}

	// If slack is requested, make a subfile
	if ( slack && istream )
	{
		CBlockStream *tempstream = new CBlockStream;
		if ( !istream->MakeSubFile(tempstream, istream->Length(), slacksize) )
		{
			delete tempstream;
			delete istream;
			return NULL;
		}
		delete istream;
		istream = tempstream;
	}
	return istream;
}

void CMFTRecord::mergedeletedrun(fssize_t blocknum, fssize_t streamrunlen, CBlockStream *stream)
{
	do
	{
		bool b;
		fssize_t liverunlen = m_ntfs->getallocationrun(blocknum, streamrunlen, b);

		if ( (b == false) && (liverunlen >= streamrunlen) )		// its not allocated and its long enough
		{
			stream->AddRun(blocknum, streamrunlen);
			blocknum += streamrunlen;
			streamrunlen = 0;
		} else
		{
			if ( b == false )
				stream->AddRun(blocknum, liverunlen);	// its not allocated, so add it
			else
				stream->AddRun(-1, liverunlen);		// it is allocated, so add a sparse run

			blocknum += liverunlen;
			streamrunlen -= liverunlen;
		}
	} while ( streamrunlen > 0 );
}

const void *CMFTRecord::getresidentattribute(int index, bool mkcopy) const
{
	if ( !isvalid() || !isvalidattributenum(index) ) return NULL;

	AttribInfo ai = m_attributes[index];
	SMFTAttribute *fa = ai.fragments[0].data;
	if ( !fa || !fa->isresident() ) return NULL;

	if ( mkcopy )
	{
		void *result = malloc(fa->r.streamlength);
		if ( !result ) return NULL;

		memcpy(result, fa->residentstream(), fa->r.streamlength);
		return result;
	} else
	{
		return fa->residentstream();
	}
}

const void *CMFTRecord::getresidentattribute(int attributetype, const wchar_t *name, int identifier, bool mkcopy) const
{
	int i = findattribute(attributetype, name, identifier);
	if ( i < 0 ) return NULL;

	return getresidentattribute(i, mkcopy);
}

fssize_t CMFTRecord::residentattributeoffset(int index)
{
	return 0;
}

#if 0
int CMFTRecord::subrecordcount() const
{
	return m_records.size();
}
MFT_RECNUM CMFTRecord::getsubrecordnum(int i)
{
	return m_records[i].recnum;
}

SMFTRecord *CMFTRecord::getsubrecordbyindex(int i)
{
	return m_records[i].record;
}
#endif

SMFTRecord *CMFTRecord::getsubrecord(MFT_RECNUM recnum)
{
	for(unsigned int i=0; i < m_records.size(); i++)
	{
		if ( m_records[i].recnum.RecNum() == recnum.RecNum() ) return m_records[i].record;
	}

	SMFTRecord *newrec = m_mft->readrawrecord(recnum);
	if ( !newrec ) return NULL;
	if ( newrec->baserecnum.RecNum() != m_baserecnum.RecNum() || m_baserec->isinuse() != newrec->isinuse() )
	{
		free(newrec);
		return NULL;
	}
	m_records.push_back( SubRecordInfo(recnum, newrec) );
	return newrec;
}

int CMFTRecord::attributecount() const
{
	return m_attributes.size();
}

int CMFTRecord::findattribute(int attributetype, const wchar_t *name, int identifier, int previous) const
{
	if ( !isvalid() ) return -1;

	wstring wname = name ? name : L"";

	for(int i=previous+1, count = m_attributes.size(); i < count; i++)
	{
		int at = m_attributes[i].attributetype;
		int id = m_attributes[i].identifier;
		wstring n = m_attributes[i].name;
		if
		(
			( attributetype == -1 || at == attributetype ) &&
			( identifier == -1 || id == identifier ) &&
			( name == NULL || wname == n )
		)
		{
			return i;
		}
	}
	return -1;
}

SMFTAttribute *CMFTRecord::getattribute(int i, int j)
{
	if ( !isvalid() || !isvalidattributenum(i) ) return NULL;
	return m_attributes[i].isvalidfragmentnum(j) ? m_attributes[i].fragments[j].data : NULL;
}

const SMFTAttribute *CMFTRecord::getattribute(int i, int j) const
{
	if ( !isvalid() || !isvalidattributenum(i) ) return NULL;
	return m_attributes[i].isvalidfragmentnum(j) ? m_attributes[i].fragments[j].data : NULL;
}

SMFTAttribute *CMFTRecord::getattribute(int attributetype, const wchar_t *name, int identifier)
{
	int i = findattribute(attributetype, name, identifier);
	return i >= 0 ? m_attributes[i].fragments[0].data : NULL;
}


int CMFTRecord::getattributetype(int index) const
{
	const SMFTAttribute *fa = getattribute(index, 0);
	return fa ? fa->attributetype : atEND;
}

wstring CMFTRecord::getattributename(int index) const
{
	const SMFTAttribute *fa = getattribute(index, 0);
	return fa ? fa->getname() : wstring();
}

int CMFTRecord::getattributeid(int index) const
{
	const SMFTAttribute *fa = getattribute(index, 0);
	return fa ? fa->identifier : -1;
}

bool CMFTRecord::getfilename(wstring &filename, vector<wstring> &filenamealiases, MFT_RECNUM &parentrec)
{
	// map filenamespace to desirablilty index, higher number being more desirable
	const static int filenamespacedesirability[] = { 3, 2, 0, 1 };

	int i, fnindex=-1;
	int prevdesire = -1;

	filename = L"";
	filenamealiases.clear();
	parentrec.data = -1;

	// Scan for the most descriptive file name this file has (ie. posix, then win32, then dos)
	for(i = findattribute(atFILENAME, NULL, -1); i >= 0; i = findattribute(atFILENAME, NULL, -1, i) )
	{
		SFilenameAttrib *fna = (SFilenameAttrib*)getresidentattribute(i, false);
		if ( !fna ) continue;
		if ( filenamespacedesirability[fna->filenamespace] > prevdesire )
		{
			fnindex = i;
			prevdesire = filenamespacedesirability[fna->filenamespace];
		}
	}

	// Add the filename fnindex as primary filename, the others as aliases
	if ( fnindex < 0 ) return false;
	for(i = findattribute(atFILENAME, NULL, -1); i >= 0; i = findattribute(atFILENAME, NULL, -1, i) )
	{
		SFilenameAttrib *fna = (SFilenameAttrib*)getresidentattribute(i, false);
		if ( !fna ) continue;

		if ( i == fnindex )
		{
			parentrec = fna->dirlocation;
			filename = fna->getfilename();
		}
		else
		{
			filenamealiases.push_back(fna->getfilename());
		}
	}
	return true;
}

}		// end namespace NTFS
}		// end namespace AccessData

