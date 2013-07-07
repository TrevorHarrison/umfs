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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "ADIOBlockDevice.h"
#include "JobCallBack.h"
#include "Path.h"
#include <vector>

namespace AccessData
{

using std::vector;

// fwd defines
class CFile;
class CDirectory;


// CFTKFileSystem
// This class defines the base interface of a file system.
// Implementations of file systems need to provide
//   1) Detect() : a way to detect a file system on a block device
//   2) Clone() : a way to make a file system object of the same type
//   3) Mount(dev) : a way to open (mount) the file system on dev
class CFileSystem : public CFTKBlockDevice
{
public:
	enum EQueryUFIDOptions
	{
    	INCLUDENONE		= 0x0000,			// include no files
		INCLUDEFILES	= 0x0001,			// include files
	    INCLUDESLACK	= 0x0002,			// include the slack at the end of files
	    INCLUDEDIRS		= 0x0004,			// include directories
	    INCLUDESPECIAL	= 0x0008,			// include special files like fsslack and unalloc
        INCLUDEDELETED	= 0x0010,			// include deleted files
        INCLUDEALL		= 0xFFFF,			// include all files
	};
	//
	// CFTKFileSystem pure virtual methods
	//

	// OpenFile()
	// Returns a new CFile object using fileref to locate it on the filesystem.
	// Must support the 'special' files ( unalloc, fsslack, etc) as well.
	virtual CFile*				OpenFile(UFID_t ufid) = 0;

	// GetRootDirectory()
	// Returns a new CFTKDirectory object that contains the entries in the root directory of the file system.
	virtual CDirectory*			GetRootDirectory() = 0;

	// GetDirectory()
	// Returns a new CFTKDirectory object that contains the entries in the directory pointed to by fileref.
	// Returns NULL if error, ie. if fileref isn't a directory.
	virtual CDirectory*			GetDirectory(UFID_t ufid) = 0;

	// GetRootDirectoryFileRef()
	// Returns a fileref that points to the root directory.  (pass it to ftkfsDirectoryGet() or ftkfsFileOpen())
	virtual UFID_t				GetRootDirectoryUFID() const = 0;

	// GetUnalloc()
	// Returns a fileref that points to the entire free space of the file system.
	virtual UFID_t				GetUnallocUFID() const = 0;

	// GetSlackFileRef()
	// Returns a fileref that points to the file system slack...
	// ie. the space between the end of the file system and the end of the partition/physical device that the file system lives on.
	virtual UFID_t				GetSlackUFID() const = 0;

    // QueryUFIDs()
    // Adds all ufids to the passed-in vector.
    // Replacement for ftkfsEnumerationGet()
    virtual bool				QueryUFIDs(vector<UFID_t> &ufids, int queryoptions) = 0;

    // GetParentUFID()
    // Returns the parent ufid of the passed-in ufid.
    // Returns -1 if there is no parent or an error
    virtual UFID_t				GetParentUFID(UFID_t ufid) = 0;

    virtual EFileType			GetFileType(UFID_t ufid) = 0;

	// IsBlockAllocated()
	// Returns true or false if blocknum is considered 'inuse' by the file system.
	virtual bool				IsBlockAllocated(INT64 blocknum) = 0;

	// IsBlockUnallocated()
	// Returns true of false if the blocknum is not considered 'inuse' by the filesystem.
	virtual bool				IsBlockUnallocated(INT64 blocknum) = 0;

	// FreeBlockCount()
	virtual INT64				FreeBlockCount() = 0;

	// UsedBlockCount()
	virtual INT64				UsedBlockCount() = 0;

	// FileSystemType()
	virtual EFileSystemType		FileSystemType() const = 0;


	//
	// File System creation and setup
	//

	// ftkfsDetect()
	// Returns a boolean indicating if the filesystem on the blockdevice
	// is supported by this CFTKFileSystem instance.
	virtual bool				Detect(CFTKBlockDevice *dev) = 0;

	// ftkfsClone()
	// Virtual constructor for a specific CFTKFileSystem
	virtual CFileSystem*		Clone() = 0;

	// ftkfsOpen()
	// Tries to open the file system on dev.
	virtual bool				Mount(CFTKBlockDevice *dev, const CPath &indexdir, CJobCallBack &callback) = 0;

	// ftkfsVerify()
	// Verifies that the file system is intact.
	// This will probably be an expensive process, but
	// it will probably be required so that the file system can have a place to map
	// out allocated vs. unallocated vs. deleted space
	// virtual bool ftkfsVerify() = 0;

	//
	// end CFTKFileSystem pure virtual methods
	//
private:
	typedef CFTKBlockDevice inherited;
};

};		// end namespace

#endif
