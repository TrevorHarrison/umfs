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

#ifndef MFTSTRUCTS_H
#define MFTSTRUCTS_H

#include "IntTypes.h"
#include "StringTypes.h"
#include "MFT_RECNUM.h"
#include "ADStream.h"
#include <wchar.h>

namespace AccessData
{
namespace NTFS
{

struct SMFTAttribute;
struct NTFSfileruns;

#pragma pack(push,1)

// This structure represents a record in the MFT file.
struct SMFTRecord
{												// offset	description
	enum { RECSIG = 0x454c4946 };				//			The same as "FILE"
	INT32			recsig;						// 0		should be "FILE"
	UINT16			fixuplistoffset;			// 4		Fixups at the end of each physical sector
	UINT16			fixuplistcount;				// 6		the number of (2 byte ints) elements in the fixup list
	UINT32			unknown1;					// 8
	UINT32			unknown2;					// c
	UINT16			sequencenumber;				// 10		used to insure that a FileRef isn't pointing to a record that has been deleted and reused.
	UINT16			referencecount;				// 12
	UINT16			attributeoffset;			// 14
	UINT16			flags;						// 16
	UINT32			recordlength_used;			// 18
	UINT32			recordlength_allocated;		// 1c		the size of this record
	MFT_RECNUM		baserecnum;					// 20		a ref to the base file record
	UINT16			maxattribid;				// 28

	// checks the recsig field to determine that this is a valid record
	bool			isvalid() const				{ return (recsig == RECSIG); }
	bool			isinuse() const				{ return flags & 1; }
	bool			isfile() const				{ return (flags & 2) == 0; }
	bool			isdirectory() const			{ return (flags & 2) == 2; }
	bool			isbaserecord() const		{ return baserecnum.RecNum() == 0; }
	bool			dofixup(int blocksize);

	SMFTAttribute*	getfirstattribute();
	SMFTAttribute*	getnextattribute(SMFTAttribute *prev);

	// findattribute()
	// Searches (using getfirst/getnext) the list of attributes in this record
	// for one that matches attributetype and name and identifier.
	// Wildcards are okay.  The only time they should be used is if there isn't an ALA.
	// Returns a pointer to the attribute record inside this instance (ie. do not free it) if found, NULL if not found
	SMFTAttribute*	findattribute(int attributetype, const wchar_t *name, int identifier, int index);
};


// This structure represents an attribute inside a NTFSfilerecord
struct SMFTAttribute
{												// offset	description
	INT32	attributetype;						// 0		See ENTFSFileAttributeTypes
	UINT32	attributelength;					// 4
	UINT8 	nonresidentflag;					// 8
	UINT8	namelength;							// 9
	UINT16	nameoffset;							// A
	UINT16	compressedflag;						// C
	UINT16	identifier;							// E
	union
	{
		struct		// resident
		{
			UINT32	streamlength;				// 10
			UINT16	streamoffset;				// 14
			UINT16	indexedflag;				// 16
		} r;
		struct		// non-resident
		{
			UINT64	startingvcn;				// 10
			UINT64	lastvcn;					// 18
			UINT16	runlistoffset;				// 20
			UINT16	compressionengine;			// 22
			UINT8	unknown[4];					//
			UINT64	streamlength_allocated;		// 28		physical size of stream on the volume
			UINT64	streamlength_real;			// 30		logical size of stream
			UINT64	streamlength_initialized;	// 38		compressed size of the stream, same as real if not compressed
		} nr;
	};
	// end of predefined data storage for the record

	bool				isnonresident() const			{ return nonresidentflag == 1; }
	bool				isresident() const				{ return nonresidentflag == 0; }
	bool				iscompressed() const			{ return isnonresident() && (compressedflag == 1); }	// and maybe && compressionengine = 4
	bool				isencrypted() const				{ return compressedflag == 0x4000; }
	char*				residentstream()				{ return ((char *)this)+r.streamoffset; }
	UINT64				streamlength_logical() const	{ return nonresidentflag == 0 ? r.streamlength : nr.streamlength_real; }
	UINT64				streamlength_physical() const	{ return nonresidentflag == 0 ? r.streamlength : nr.streamlength_allocated; }
	const NTFSfileruns*	getruns() const					{ return isnonresident() ? (NTFSfileruns *)(((char *)this)+nr.runlistoffset) : NULL; }
	wstring				getname() const;
};

// This structure represents a NTFS sector/length compressed run list
// Use as:
// for(char *cp=abc.getfirstrun(offset, length); cp != NULL; cp = abc.getnextrun(offset, length, cp) ) { /* do stuff */ }
// However, the offset value returned by getnextrun will be relative to the previous value of offset.  The offset
// value returned by getfirstrun is absolute to the volume.
struct NTFSfileruns
{
	char runs[1];

	// getfirstrun() / getnextrun()
	// Sets offset and length to the first value stored in char runs[]
	// Returns a pointer that should be passed to getnextrun (as prev), or NULL if the end of the list was reached
	const char *getfirstrun(INT64 &offset, INT64 &length, bool &sparse) const;
	const char *getnextrun(INT64 &offset, INT64 &length, bool &sparse, const char *prev) const;
};

// This struct is a NTFS attribute list attribute record.
// It is found in the attribute list attribute stream
struct ALArec
{
	enum { MAXRECORDLENGTH = 0x200 };  // this is a wag as to a max acceptable record length so we don't alloc a huge assed buffer
										// offset	description
	INT32		attributetype;			// 0
	UINT16		recordlength;			// 4
	UINT8		namelength;				// 6
	UINT8		nameoffset;				// 7		usually 0x1A, probably name offset
	UINT64		startingvcn; 			// 8
	MFT_RECNUM	attributelocation;		// 10
	UINT16		identifier;				// 18
	//wchar_t	name[1];				// 1A		start of attr name, in unicode

	static ALArec *read(CStream *f, void *buffer=NULL, int buffersize=0);
	bool		isvalid() const;
	bool		compare(const ALArec &rhs) const;
	wstring		getname() const;
};

#pragma pack(pop)

};		// end NTFS namespace
};		// end AccessData namespace

#endif
