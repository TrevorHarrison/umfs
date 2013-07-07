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

#ifndef FTKBOOTRECORD_H
#define FTKBOOTRECORD_H

#include "IntTypes.h"
#include "ADIOTypes.h"
#include "ADIOBlockDevice.h"

namespace AccessData
{

#pragma pack(push,1)
struct SBiosParamBlock
{									// Offset	Description
	UINT16 blocksize;				// 0		size of physical blocks(ie. sectors) in bytes, usually 512
	UINT8  clustersize;				// 2		size of a cluster, in blocks	(ie. a value of 2 would give 2*512 byte blocks == 1k sectors)
	UINT16 reservedblocks;			// 3		points to first fat sector
	UINT8  FATcount;				// 5		number of fats
	UINT16 maxrootdirentries;		// 6		max number of entries in the root directory
	UINT16 blockcount;				// 8		if 0, look at blockcount2
	UINT8  mediadescriptor;			// A		f8 == fixed disk
	UINT16 FATsize;					// B		how big each fat table is in blocks
	UINT16 blockspertrack;			// D		number of blocks per track
	UINT16 headcount;				// F		number of heads
	UINT32 hiddensectors;			// 11		number of hidden sectors
	UINT32 blockcount2;				// 15		holds number of blocks in the file system if blockcount == 0

	union {
		struct
		{
			UINT8 phyiscaldrivenumber;	// 19		80 == 'C'
			UINT8 reserved1;			// 1A
			UINT8 extbootrecsig;		// 1B
			UINT32 volumeserialnumber;	// 1C
			char volumelabel[11];		// 20
			char volumetype[8];			// 2B
		} fat16;
		struct
		{
			UINT32 FATsize;				// 19		size of fat in blocks
			UINT16 flags;				// 1D		bit 8: fat mirroring, low 4: active fat
			UINT8 version[2];			// 1F		major/minor ver
			UINT32 rootdircluster;
			UINT16 infocluster;
			UINT16 backupboot;
			UINT16 unknown[6];
			UINT8 phyiscaldrivenumber;
			UINT8 reserved1;
			UINT8 extbootrecsig;
			UINT32 volumeserialnumber;
			char volumelabel[11];
			char volumetype[8];
		} fat32;
		struct
		{
			UINT16 Word1;					//			always 00 80 ?
			UINT16 Word2;					//			always 00 80 ?
			UINT64 blockcount;				//			blocks in the blockdev
			UINT64 mftstart;				//			cluster number where the mft starts
			UINT64 mftmirrorstart;			//			cluster number where the mirror mft starts
			INT32  mftrecordsize;			//			in clusters, < 0 == 2^number, ie. F6 == -10 = 2^10 = 1024
			UINT32 indexbuffersize;			//			in clusters
			UINT32 volumeserialnumber;
		} ntfs;
	};

	EFileSystemType getfattype() const;
	UINT32 getfatblockcount() const { return blockcount != 0 ? blockcount : blockcount2; }
	UINT32 getfattablesize(EFileSystemType fst) const { return fst == fsFAT32 ? fat32.FATsize : FATsize; }
	string getvolumelabel(EFileSystemType fst) const;
	UINT32 getvolumeserialnumber(EFileSystemType fst) const;
};

// a single entry in a partition table
struct SPartitionRecord
{								// Offset	Descripition
	UINT8 bootindicator;		// 0		0 = nonbootable, 0x80 = bootable
	UINT8 starthead;			// 1		starting head
	UINT8 startsector;			// 2		starting sector (bits 0-5.  bits 6-7 are bits 8 and 9 for starting cylinder)
	UINT8 startcylinder;		// 3		starting cylinder
	UINT8 type;					// 4		partition type, ie. 1 = FAT12, 4 = FAT16, etc
	UINT8 endhead;				// 5		ending head
	UINT8 endsector;			// 6		ending sector (bits 0-5... see starting sector)
	UINT8 endcylinder;			// 7		ending cylinder
	UINT32 firstsector;			// 8		the sector number (relative to the sector where this record is stored) where this partition starts
	UINT32 sectorcount;			// C		the number of sectors in the partition

	bool isvalid(fssize_t maxsectors) const;
	bool isbootable() const { return bootindicator == 0x80; }
	bool isblank() const { return (((const UINT64 *)this)[0] == 0) && (((const UINT64 *)this)[1] == 0); }

	UINT16 getstartcylinder() const { return (((UINT16)startsector & 0xC0) << 8) | startcylinder; }
	UINT8 getstarthead() const { return starthead; }
	UINT8 getstartsector() const { return startsector & 0x3f; }

	UINT16 getendcylinder() const { return (((UINT16)endsector & 0xC0) << 8) | endcylinder; }
	UINT8 getendhead() const { return endhead; }
	UINT8 getendsector() const { return endsector & 0x3f; }
};

// Can either be a volume boot record (vbr) or a master boot record (mbr).
// Only VBR's have a bpb.
// Only MBR's have the partition table
struct SBootRecord
{									// Offset	Description
	UINT8 jumpins[3];				// 0		initial jump instruction to jump over the bpb to the real code
	char sig[8];					// 3		a text signature
	union
	{
		SBiosParamBlock bpb;		// B		the BIOS parameter block.  Only present in volume boot records, not the mbr
		char raw1[0x1b3];			//			padding	
	};
	SPartitionRecord partitions[4];	// 1b0		the partition table.  Only present in the mbr or ext boot records
	UINT16 magiccookie;				// 1fe		AA55

	enum { MAXPARTITIONRECORDS = 4 };

	bool Read(CFTKBlockDevice *dev, fssize_t blocknum = 0);
	bool isvalid() const;
	bool isvalidntfs() const;
	bool isvalidfat() const;
	bool haspartitiontable(fssize_t maxsectors) const;

	EFileSystemType FileSystemTypeDetect() const;

	int getbootpartition() const;
};
#pragma pack(pop)

};		// end namespace

#endif
