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

#ifndef NTFSATTRIBUTESTRUCTS_H
#define NTFSATTRIBUTESTRUCTS_H

#include "ADStream.h"
#include "StringTypes.h"
#include "MFT_RECNUM.h"
#include "ADIOTypes.h"

#include <time.h>

namespace AccessData
{
namespace NTFS
{

#pragma pack(push,1)
struct SStandardInfoAttrib		// atSTANDARDINFORMATION
{
	enum
	{
		NTFS_ATTRIB_RDONLY		= 0x00000001,
		NTFS_ATTRIB_HIDDEN		= 0x00000002,
		NTFS_ATTRIB_SYSTEM		= 0x00000004,
		NTFS_ATTRIB_ARCHIVE		= 0x00000020,
		NTFS_ATTRIB_SYMLINK		= 0x00000400,
		NTFS_ATTRIB_COMPRESSED	= 0x00000800,
		NTFS_ATTRIB_ENCRYPTED	= 0x00004000,
		NTFS_ATTRIB_DIRECTORY	= 0x20000000,
	};

	UINT64	createtime;
	UINT64	lastmodtime;
	UINT64	filereclastmodtime;
	UINT64	accesstime;
	UINT32	dosperms;

	time_t	getctime() const;
	time_t	getmtime() const;
	time_t	getmmtime() const;
	time_t	getatime() const;

	bool	isreadonly() const		{ return (dosperms & NTFS_ATTRIB_RDONLY) != 0; }
	bool	ishidden() const		{ return (dosperms & NTFS_ATTRIB_HIDDEN) != 0; }
	bool	issystem() const		{ return (dosperms & NTFS_ATTRIB_SYSTEM) != 0; }
	bool	isarchive() const		{ return (dosperms & NTFS_ATTRIB_ARCHIVE) != 0; }
	bool	issymlink() const		{ return (dosperms & NTFS_ATTRIB_SYMLINK) != 0; }
	bool	iscompressed() const	{ return (dosperms & NTFS_ATTRIB_COMPRESSED) != 0; }
	bool	isencrypted() const		{ return (dosperms & NTFS_ATTRIB_ENCRYPTED) != 0; }
	bool	isdirectory() const		{ return (dosperms & NTFS_ATTRIB_DIRECTORY) != 0; }
};


struct SFilenameAttrib
{											// offset	description
	MFT_RECNUM	dirlocation;				// 0		Points to the MFT record num of the directory where this filename lives
	INT64		createtime;
	INT64		lastmodtime;
	INT64		filereclastmodtime;
	INT64		accesstime;
	INT64		filelength_physical;		// 28
	INT64		filelength_logical;			// 30
	INT64		flags;						// 38
	UINT8		filenamelength;				// 40
	UINT8		filenamespace;				// 41
	wchar_t		filename[1];				// 42

	// bit masks for the flags field
	enum { flagDIRECTORY = 0x10000000, flagCOMPRESSED = 0x800, flagARCHIVE = 0x20, flagSYSTEM = 0x4, flagHIDDEN = 0x2, flagREADONLY = 0x1 };

	static SFilenameAttrib*		Read(CStream *f);	// caller must free the pointer
	wstring						getfilename();
	EFilenameType				getfilenametype();
};
#pragma pack(pop)

}		// end namespace NTFS
}		// end namespace AccessData


#endif
