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

#ifndef NTFSINDEXSTRUCTS_H
#define NTFSINDEXSTRUCTS_H

#include "IntTypes.h"
#include "ADStream.h"
#include "MFT_RECNUM.h"
#include "NTFSattributestructs.h"
#include <wchar.h>

namespace AccessData
{
namespace NTFS
{

struct NTFSindexentry;

#pragma pack(push,1)

// This struct is common between NTFS indexroots and indexnodes
// In Regis' docs, these fields are listed in each of the structs,
// with a caviat that you need to add 0x10 or 0x18 to the values.
// That additional offset value happens to be the same as the offset
// of the first of that set of fields.
// This indicated that these values were relative to an object
// that was located there, and that object happened to start at 0x10
// or 0x18.
struct NTFSindexentrylist
{								// offset	description
	UINT32 liststart;			// 0		offset (from this object) to the start of the indexentrylist
	UINT32 listend;				// 4		offset to just past the last indexentry
	UINT32 listsize;			// 8		offset to the end of the list

	bool			isvalid();

	NTFSindexentry*	getfirstentry();
	NTFSindexentry*	getnextentry(NTFSindexentry *prev);

	// findentry()
	// Searches the indexentrylist for an indexentry that matches name.
	// If it doesn't find it, but does find a subnode, that subnode will be returned and
	// issubnode will be set to true.
	// Returns:
	//   NULL if not found,
	//   pointer to the indexentry if found, and insubnode is set to false
	//   pointer to the subnode entry if possibly in subnode, and insubnode set to true
	NTFSindexentry*	findentry(const wchar_t *name, bool &insubnode);

};

// The root node of the index btree.
// Attribute 0x90, always resident in the MFT.
// Call indexroot.indexentries.findentry(...) to start searching for a filename
struct NTFSindexroot
{												// offset	description
	INT32				attributetype;			// 0		the type of the indexed attribute
	INT32				unknown1;				// 4		always 1
	UINT32				indexnodesize;			// 8		size of the indexnodes (in bytes) in the index allocation attribute
	UINT32				clustersperindexbuffer;	// C
	NTFSindexentrylist	indexentries;			// 10
	UINT32				flags;					// 1C

	// virtual ctor... returns a pointer to a malloc'd NTFSindexroot of the appropriate size
	static NTFSindexroot*	Read(CStream *f);
	bool					isvalid();
	bool					islargeindex();
};


// Subnode in the index btree.
// Stored in attribute 0xA0.
// This is the only index struct that needs a dofixup()
struct NTFSindexnode
{
	enum { INDEXRECSIG = 0x58444e49 };
											// offset	description
	INT32				indexsig;			// 0
	UINT16				fixuplistoffset;	// 4
	UINT16				fixuplistcount;		// 6
	UINT8				unknown[8];			//
	INT64				selfblock;			// 10		this record's block number in the file.  After loading into memory, this can be used for parent block number
	NTFSindexentrylist	indexentries;		// 18
	UINT32				flags;				// 24		0 = leaf, 1 == branch

	static NTFSindexnode*	Read(CStream *f, INT64 nodenum, int indexnodesize, int blocksize);
	bool					isvalid();
	bool					dofixup(int mysize, int blocksize);
};


// This struct stores the actual data that is being indexed.
// In the normal (only?) case, its a filename attribute (type 0x30)
// The type is stored in the indexroot
struct NTFSindexentry
{								// offset	description
	MFT_RECNUM	fileref;		// 0
	UINT16		reclength;		// 8
	UINT16		datalength;		// A
	UINT8		flags;			// C
	UINT8		unknown[3];		//			probably part of flags
	UINT8		data[1];		// 10		this is where the filename attribute is stored
	//SFilenameAttrib	filenameattribute;	// this is only present when islast() == false
	//INT64				subnodevcn;			// reclength-8

	bool				isvalid();
	bool				issubnode()			{ return (flags & 1) == 1; }
	bool				islast()			{ return (flags & 2) == 2; }

	INT64				getsubnodeblock();						// this is only present when issubnode() == true
	SFilenameAttrib&	getfilenameattribute() { return *(SFilenameAttrib *)data; }
};


#pragma pack(pop)

}		// end namespace NTFS
}		// end namespace AccessData

#endif
