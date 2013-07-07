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

#ifndef FILE_H
#define FILE_H

#include "ADIOtypes.h"
#include "BlockStream.h"
#include "Path.h"
#include "StringTypes.h"
#include <vector>
#include <time.h>

namespace AccessData
{

using std::vector;

enum EFileFlags
{
	DF_READONLY		= 0x000001,
    DF_HIDDEN		= 0x000002,
    DF_SYSTEM		= 0x000004,
    DF_ARCHIVE		= 0x000008,
    DF_COMPRESSED	= 0x000010,
    DF_ENCRYPTED	= 0x000020,

    FF_DELETED		= 0x001000,
    FF_DIRECTORY	= 0x002000,
    FF_FILE			= 0x004000,
};

// CFile
class CFile
{
public:
	CFile();
	virtual ~CFile();

	// Stream access stuff
	virtual CBlockStream*	Open() = 0;
	virtual CBlockStream*	OpenSlack() = 0;

	// Get the file's unique id
	virtual UFID_t			GetUFID() const = 0;
    virtual	UFID_t			GetParentUFID() const = 0;

    // Get the file's name and path
    virtual	wstring			GetName() const = 0;		// returns just filename
    virtual CPath			GetPath() const = 0;		// returns full path + filename
    virtual void			GetNames(vector<wstring> &filenames)const = 0;
    virtual void			GetPaths(vector<CPath> &paths) const = 0;

	virtual EFileType		GetFileType() const = 0;

    virtual	INT64			Length() const = 0;
    virtual	INT64			PhysicalLength() const = 0;

    virtual	time_t			CreateDate() const = 0;
    virtual	time_t			ModifyDate() const = 0;
    virtual	time_t			AccessDate() const = 0;

    virtual	UINT32			Flags() const = 0;
	bool					IsDirectory() const			{ return (Flags() & FF_DIRECTORY) != 0; }
	bool					IsFile() const				{ return (Flags() & FF_FILE) != 0; }
	bool					IsDeleted() const			{ return (Flags() & FF_DELETED) != 0; }

protected:
private:
	CFile(const CFile &rhs);		// disallow
    CFile &operator=(const CFile &rhs);	// disallow
};

}		// end namespace

#endif