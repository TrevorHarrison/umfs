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

#include "NTFScommon.h"

namespace AccessData
{
namespace NTFS
{

bool dofixup(void *rec, int recsize, int blocksize, int fixupcount, UINT16 *fixups)
{
	UINT16 magiccookie = *fixups;
	fixups++;

	int bc = recsize / blocksize;
	if ( fixupcount-1 != bc ) return false;

	for(int i=0; i < bc; i++)
	{
		UINT16 *p = (UINT16 *)( ((char *)rec) + (blocksize*(i+1)) - 2 );

		if ( *p != magiccookie ) return false;
		*p = fixups[i];
	}
	return true;
}

#pragma pack(push,1)
struct SNTFSUFID
{
	UINT8 flags;
	UINT8 reserved;
	UINT16 attribnum;
	UINT32 recnum;
	enum { ISSLACK = 1 };
};
#pragma pack(pop)

UFID_t ntfs2ufid(UINT16 attribnum, UINT64 recnum, bool isslack)
{
	UFID_t ufid = 0;
	SNTFSUFID &ntfsufid = *((SNTFSUFID *)(&ufid));

	ntfsufid.attribnum = attribnum;
	ntfsufid.recnum = (UINT32)(recnum & 0xFFFFFFFFL);
	ntfsufid.flags |= isslack ? SNTFSUFID::ISSLACK : 0;

	return ufid;
}

void ufid2ntfs(UFID_t ufid, UINT16 &attribnum, UINT64 &recnum, bool &isslack)
{
	SNTFSUFID &ntfsufid = *((SNTFSUFID *)(&ufid));

	recnum = ntfsufid.recnum;
	attribnum = ntfsufid.attribnum;
	isslack = (ntfsufid.flags & SNTFSUFID::ISSLACK) == SNTFSUFID::ISSLACK;
}

#define NTFS_TIME_FUDGE (0x019db1ded53e8000L)	// subtract this number from an NTFS 64bit time value to justify it to Jan 1, 1970
#define NTFS_TIME_PERSEC (10000000)				// this is the number of ntfs time units per second (10million)
time_t ntfstime2time_t(UINT64 ntfstime)
{
	ntfstime -= NTFS_TIME_FUDGE;
	time_t result = ntfstime / NTFS_TIME_PERSEC;
	return result;
}


}		// end namespace NTFS
}		// end namespace AccessData


