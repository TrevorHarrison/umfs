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

#ifndef ADSTREAM_H
#define ADSTREAM_H

#include "IntTypes.h"

namespace AccessData
{

// CStream
// This is the base interface for a random access file/stream.
class CStream
{
public:
	virtual ~CStream();
	// isvalid()
	// Returns true if the instance is in a good state
	virtual bool			isvalid() const = 0;

	// clear()
	// Resets the instance to default values
	virtual void			clear() = 0;

	// Dup()
	// Returns an identical copy this instance.
	virtual CStream*		Dup() const = 0;

	// ftksioRead()
	// Reads bytestoread bytes from the the stream (either from pos or the current file position) into dest.
	// Returns the number of bytes actually read.
	virtual int				Read(void *dest, int bytestoread) = 0;
	virtual int				Read(void *dest, int bytestoread, INT64 pos) = 0;

	// eof
	// Returns true if the current file position is at or past the end of the file
	virtual bool			Eof() = 0;

	// seek()
	// Changes the current position in the file.  Indicate where and how
	// to move the current position with amount and whence.
	// Returns the new position of the file.
	enum SEEK_WHENCE { swSET, swCUR, swEND };
	virtual INT64			Seek(INT64 amount, SEEK_WHENCE whence) = 0;

	// tell()
	// Returns the current position in the file.
	virtual INT64			Tell() const = 0;

	virtual INT64			Length() const = 0;

};

}		// end namespace

#endif
