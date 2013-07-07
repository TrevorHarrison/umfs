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

#ifndef NTFSCOMMON_H
#define NTFSCOMMON_H

#include "IntTypes.h"
#include "ADIOTypes.h"
#include <time.h>

namespace AccessData
{
namespace NTFS
{

enum ENTFSFileAttributeTypes
{
	atSTANDARDINFORMATION		= 0x10,
	atATTRIBUTELIST				= 0x20,
	atFILENAME					= 0x30,
	atVOLUMEVERSION				= 0x40,
	atSECURITYDESCRIPTOR		= 0x50,
	atVOLUMENAME				= 0x60,
	atVOLUMEINFORMATION			= 0x70,
	atDATA						= 0x80,
	atINDEXROOT					= 0x90,
	atINDEXALLOCATION			= 0xA0,
	atBITMAP					= 0xB0,
	atSYMBOLICLINK				= 0xC0,
	atEAINFORMATION				= 0xD0,
	atEA						= 0xE0,
	atEND						= 0xFFFFFFFF
};

enum ENTFSSystemFileRecordNumbers
{
	sfrMFT				= 0,
	sfrMFTMirr			= 1,
	sfrLogFile			= 2,
	sfrVolume			= 3,
	sfrAttrDef			= 4,
	sfrRootDir			= 5,
	sfrBitmap			= 6,
	sfrBoot				= 7,
	sfrBadClusters		= 8,
	sfrQuota			= 9,
	sfrUpCase			= 10,
	sfrReserved1		= 11,
	sfrReserved2		= 12,
	sfrReserved3		= 13,
	sfrReserved4		= 14,
	sfrReserved5		= 15
};

bool	dofixup(void *rec, int recsize, int blocksize, int fixupcount, UINT16 *fixups);
UFID_t	ntfs2ufid(UINT16 attribnum, UINT64 recnum, bool isslack);
void	ufid2ntfs(UFID_t ufid, UINT16 &attribnum, UINT64 &recnum, bool &isslack);
time_t	ntfstime2time_t(UINT64 ntfstime);

}		// end namespace NTFS
}		// end namespace AccessData

#endif