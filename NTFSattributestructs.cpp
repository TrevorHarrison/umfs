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

#include "NTFSattributestructs.h"
#include "NTFSCommon.h"
#include "ADIOString.h"
#include <malloc.h>
#include <string.h>

namespace AccessData
{
namespace NTFS
{

time_t SStandardInfoAttrib::getctime() const
{
	return ntfstime2time_t(createtime);
}

time_t SStandardInfoAttrib::getmtime() const
{
	return ntfstime2time_t(lastmodtime);
}

time_t SStandardInfoAttrib::getmmtime() const
{
	return ntfstime2time_t(filereclastmodtime);
}

time_t SStandardInfoAttrib::getatime() const
{
	return ntfstime2time_t(accesstime);
}

//--------------------------------------------------------------------------------------------------------------------------

SFilenameAttrib *SFilenameAttrib::Read(CStream *f)
{
	if ( !f || !f->isvalid() ) return NULL;

	int fsize = f->Length();
	SFilenameAttrib *temp = (SFilenameAttrib *)malloc( fsize );
	if ( !temp ) return NULL;

	if ( f->Read(temp, fsize, 0) != fsize )
	{
		free(temp);
		return NULL;
	}
	return temp;
}

wstring SFilenameAttrib::getfilename()
{
	return wz2w(filename, filenamelength);
}

EFilenameType SFilenameAttrib::getfilenametype()
{
	switch ( filenamespace )
	{
		case 0: return fntUNIX;
		case 1: case 3: return fntWIN32;
		case 2: return fntDOS;
		default: return fntUnknown;
	}
}

}		// end namespace NTFS
}		// end namespace AccessData
