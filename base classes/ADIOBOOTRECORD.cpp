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

#include "ADIOBootRecord.h"

namespace AccessData
{

#define FAT32_TYPESTRING "FAT32   "
#define FAT16_TYPESTRING "FAT16   "
#define NTFS_TYPESTRING  "NTFS    "
#define FAT12_CLUSTERSPERFATBLOCK 341
EFileSystemType SBiosParamBlock::getfattype() const
{
	if ( FATsize == 0 && maxrootdirentries == 0 && fat32.FATsize != 0 && strncmp(fat32.volumetype, FAT32_TYPESTRING, 8) == 0 ) return fsFAT32;
	if ( FATsize == 0 || (blockcount == 0 && blockcount2 == 0) || clustersize == 0 ) return fsUnknown;
	if ( strncmp(fat16.volumetype, FAT16_TYPESTRING, 8) == 0 ) return fsFAT16;
	if (
			blockcount <= FATsize * FAT12_CLUSTERSPERFATBLOCK * clustersize && 
			blockcount >= (FATsize-1) * FAT12_CLUSTERSPERFATBLOCK * clustersize 
		) 
		return fsFAT12;
	return fsUnknown;
}

string SBiosParamBlock::getvolumelabel(EFileSystemType fst) const
{
	const char *s = NULL;
	if ( fst == fsFAT12 || fst == fsFAT16 )
		s = &fat16.volumelabel[0];
	else if ( fst == fsFAT32 )
		s = &fat32.volumelabel[0];
	if ( !s ) return string();

    int i;
	for(i=10; i > 0 && s[i] == ' '; i--) { /* nada */ }
	if ( s[i] == ' ' ) return string();

	return string(s, i+1);	// NEEDS TESTING
}

UINT32 SBiosParamBlock::getvolumeserialnumber(EFileSystemType fst) const
{
	switch ( fst )
	{
		case fsFAT12:
		case fsFAT16:
			return fat16.volumeserialnumber;
		case fsFAT32:
			return fat32.volumeserialnumber;
		case fsNTFS:
			return ntfs.volumeserialnumber;
	}
	return 0;
}

bool SPartitionRecord::isvalid(fssize_t maxsectors) const
{
	UINT32 startx = (getstartcylinder() << 16) | (getstarthead() << 8) | getstartsector();
	UINT32 endx = (getendcylinder() << 16) | (getendhead() << 8) | getendsector();

	return
		isblank() ||
		(
			// had to remove the next two tests because of real-world data:
			// 1) ran into a entry with sectorcount == 0
			// 2) ran into an encase image that truncated the last few meg of the disk
			//(sectorcount > 0) &&
			//(firstsector+sectorcount <= maxsectors) &&
			(type != 0) &&
			(bootindicator == 0x00 || bootindicator == 0x80) //&&
			//(startx < endx)
		);
}

bool SBootRecord::Read(CFTKBlockDevice *dev, fssize_t blocknum)
{
	if ( !dev || !dev->isvalid() || dev->ftkbioBlockSize() < sizeof(SBootRecord) ) return false;
	return dev->ftkbioBlockRead(this, blocknum, 0, sizeof(SBootRecord)) && isvalid();
}

#define BOOTRECORD_MAGICCOOKIE (0xAA55)
bool SBootRecord::isvalid() const
{
	return magiccookie == BOOTRECORD_MAGICCOOKIE;
}

bool SBootRecord::isvalidntfs() const
{
	return strncmp(sig, NTFS_TYPESTRING, sizeof( sig ) ) == 0 && bpb.clustersize > 0 && bpb.ntfs.blockcount > 0;
}

bool SBootRecord::isvalidfat() const
{
	return bpb.getfattype() != fsUnknown;
}

bool SBootRecord::haspartitiontable(fssize_t maxsectors) const
{
	for(int i=0; i < MAXPARTITIONRECORDS; i++)
	{
		if ( !partitions[i].isvalid(maxsectors) ) return false;
	}
	return true;
}

EFileSystemType SBootRecord::FileSystemTypeDetect() const
{
	if ( magiccookie != BOOTRECORD_MAGICCOOKIE ) return fsUnknown;
	if ( strncmp(sig, NTFS_TYPESTRING, sizeof( sig ) ) == 0 ) return fsNTFS;
	EFileSystemType temp = bpb.getfattype();
	if ( temp != fsUnknown ) return temp;
	return fsUnknown;
}

int SBootRecord::getbootpartition() const
{
	for(int i=0; i < MAXPARTITIONRECORDS; i++)
	{
		if ( partitions[i].isbootable() )  return i;
	}
	return -1;
}

};		// end namespace
