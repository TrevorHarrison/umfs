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

#ifndef MFT_H
#define MFT_H

#include "MFT_RECNUM.h"
#include "ADIOBootRecord.h"
#include "NTFSBitmap.h"
#include "ADStream.h"
#include "BlockStream.h"
#include <vector>

namespace AccessData
{
namespace NTFS
{

using std::vector;

// fwd defines
class CNTFS;
class CMFTRecord;
struct SMFTRecord;
struct SMFTAttribute;

class CMFT
{
public:
	CMFT();
	~CMFT();

	bool		isvalid() const;
	void		clear();

	bool		open(CNTFS *ntfs, CBlockStream *mftstream, int recsize, int physicalblocksize);	// assumes ownership of mftstream
	bool		bootstrap(CNTFS *ntfs, SBootRecord *bootrec);

    CMFTRecord*	readrecord(MFT_RECNUM recnum);
    SMFTRecord*	readrawrecord(MFT_RECNUM recnum);

	int			recordcount() const		{ return m_reccount; }
	int			recordsize() const		{ return m_recsize; }
    int			recordoverhead() const	{ return m_recoverhead; }

    int			freecount();
    int			alloccount();
    int			reservecount()			{ return 8; }

	CStream*		getstream()				{ return m_stream; }
	CNTFSBitmap*	getbitmap();
    bool			getrecinfo(MFT_RECNUM recnum, INT64 &startcluster, int &length);
    bool			isvalidrecnum(MFT_RECNUM recnum) const	{ return recnum.RecNum() >= 0 && recnum.RecNum() < m_reccount; }
protected:
	void		initfields();
	void		clearfields();
	void		assignfields(const CMFT &rhs);

	CNTFS*			m_ntfs;
	CBlockStream*	m_stream;

	int			m_physicalblocksize;	// the physical block size of the drive, needed to correctly do fixups
	int			m_recsize;				// the record size (in bytes) of a mft file record
	int			m_reccount;				// the number of records in the mft stream
    int			m_recoverhead;
private:
	CMFT(const CMFT &rhs);		// disallow
	CMFT &operator=(const CMFT &rhs);	// disallow
	//void		assign(const CMFT &rhs);
};


// Represents the collection of mft records that are required to define a file.
// This mostly just holds into about where the various attributes are in the various subrecords, and how to get to them.
// Could get by without being tied to a NTFS filesystem *, but for deleted file handling, need to check allocated space on the filesystem
// when constructing a stream.
class CMFTRecord
{
public:
	CMFTRecord();
	~CMFTRecord();

	bool		isvalid() const;
	void		clear();
	bool		open(CNTFS *ntfs, CMFT *mft, MFT_RECNUM recnum);


	// high level stuff
	bool					isdeleted() const;
    bool					isdirectory() const;
	MFT_RECNUM				baserecordnum() const { return m_baserecnum; }
    UINT16					seqnum() const;

	// construct a stream that points to some blocks
	CBlockStream*			openattribute(int attributetype, const wchar_t *name, int identifier, bool slack=false);
	CBlockStream*			openattribute(int index, bool slack=false);

	// Returns a pointer to a resident attribute.  Use mkcopy to get a copy of the buffer you must free.
	const void*				getresidentattribute(int index, bool mkcopy=false) const;
	const void*				getresidentattribute(int attributetype, const wchar_t *name, int identifier, bool mkcopy=false) const;

	// returns the offset of the requested attribute in the mft stream
	INT64					residentattributeoffset(int index);

	// low level stuff
	SMFTRecord*				getbaserecord() { return m_baserec; }
	SMFTRecord*				getsubrecord(MFT_RECNUM recnum);

	int						attributecount() const;
    bool					isvalidattributenum(int i) const { return i >= 0 && i < m_attributes.size(); }

	// returns the index of the requested attribute, -1 == error
	int						findattribute(int attributetype, const wchar_t *name, int identifier, int previous=-1) const;

	// returns a pointer to the attributes physical record
	SMFTAttribute*			getattribute(int i, int j);
	const SMFTAttribute*	getattribute(int i, int j) const;
	SMFTAttribute*			getattribute(int attributetype, const wchar_t *name, int identifier);

	// Returns the type/name/identifier of the index'th file attribute.
	int						getattributetype(int index) const;
	wstring					getattributename(int index) const;
	int						getattributeid(int index) const;

	bool					getfilename(wstring &filename, vector<wstring> &filenamealiases, MFT_RECNUM &parentrec);
protected:
	struct SubRecordInfo
	{
		SubRecordInfo(MFT_RECNUM rn, SMFTRecord *rp) : recnum(rn), record(rp) { }
		MFT_RECNUM		recnum;
		SMFTRecord*		record;
	};
    struct AttribFragInfo
    {
    	AttribFragInfo(MFT_RECNUM alocation, SMFTAttribute *adata) : location(alocation), data(adata) { }
    	MFT_RECNUM		location;
        SMFTAttribute*	data;
    };
	struct AttribInfo
	{
    	typedef vector<AttribFragInfo> FragInfo;
    	void		clear();
        bool		isvalidfragmentnum(int i) const		{ return i >= 0 && i < fragments.size(); }
		bool		read(SMFTAttribute *fa, MFT_RECNUM recnum);
		bool		read(CStream *fin);

		int			attributetype;
		wstring		name;
		int			identifier;
		INT64		streamlength_logical;
        INT64		streamlength_physical;
		FragInfo	fragments;
	};
	typedef vector<SubRecordInfo> RecordVector;
	typedef vector<AttribInfo> AttribVector;

	void			initfields();
	void			clearfields();
	void			mergedeletedrun(fssize_t blocknum, fssize_t streamrunlen, CBlockStream *stream);

	RecordVector	m_records;
	AttribVector	m_attributes;

	CNTFS*			m_ntfs;
	CMFT*			m_mft;
	int				m_recsize;
	MFT_RECNUM		m_baserecnum;
	SMFTRecord*		m_baserec;
private:
	CMFTRecord(const CMFTRecord &rhs);				// disallow
	CMFTRecord &operator=(const CMFTRecord &rhs);	// disallow
};

}		// end NTFS namespace
}		// end AccessData namespace

#endif
