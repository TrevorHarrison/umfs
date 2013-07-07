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

#ifndef MFT_RECNUM_H
#define MFT_RECNUM_H

#include "IntTypes.h"

namespace AccessData
{
namespace NTFS
{

// MFT_RECNUM
// This class represents a record number in the MFT.  The sequence number (SeqNum)
// is used to prevent accidental matching of record numbers if a record is deleted and then
// reused at a later time.
#pragma pack(push,1)
struct MFT_RECNUM
{									// offset	description
	//UINT32 recnumlow;				// 0		this is a strange 6 byte int, use getrecnum() to read
	//UINT16 recnumhi;				// 4
	//UINT16 seqnum;				// 6
	INT64 data;

	MFT_RECNUM() : data(-1) { }
	MFT_RECNUM(INT64 i) : data(i & 0x0000FFFFFFFFFFFFL) { }
	MFT_RECNUM(int seqnum, INT64 recnum) : data( (((fssize_t)(seqnum & 0xFFFF)) << 48) | (recnum & 0x0000FFFFFFFFFFFFL) ) { }

	bool	isvalid() const { return data != -1; }
    void	clear()		{ data = -1; }

	//bool operator==(const MFT_RECNUM &rhs) const { return data == rhs.data; }
	//bool operator!=(const MFT_RECNUM &rhs) const { return data != rhs.data; }

	INT64  RecNum() const { return data & 0x0000FFFFFFFFFFFFL; }
	UINT16 SeqNum() const { return (data & 0xFFFF000000000000L) >> 48; }

	// The seq num is always non-zero, so a 0 seq num is used to indicate a 'wildcard' that
	// matches any seq num
	bool isseqwildcard() const { return SeqNum() == 0; }
};
#pragma pack(pop)
//typedef MFT_RECNUM SFTKNTFSFileRef;

}		// end namespace NTFS
}		// end namespace AccessData

#endif
