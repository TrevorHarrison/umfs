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

#include "MFTstructs.h"

#include "NTFScommon.h"
#include "ADIOString.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>

namespace AccessData
{
namespace NTFS
{

SMFTAttribute *SMFTRecord::getfirstattribute()
{
	if ( attributeoffset == 0 ) return NULL;

	SMFTAttribute *fa = (SMFTAttribute *)(((char *)this)+attributeoffset);
	return fa;
}

SMFTAttribute *SMFTRecord::getnextattribute(SMFTAttribute *prev)
{
	if ( !prev || prev->attributetype == atEND ) return NULL;

	SMFTAttribute *fa = (SMFTAttribute *)(((char *)prev)+prev->attributelength);
	return fa;
}

SMFTAttribute *SMFTRecord::findattribute(int attributetype, const wchar_t *name, int identifier, int index)
{
	wstring wname = name ? name : L"";
	for(SMFTAttribute *fa = getfirstattribute(); fa != NULL; fa = getnextattribute(fa) )
	{
		if ( attributetype == -1 || fa->attributetype == attributetype )
		{
			if ( name == NULL || fa->getname() == wname )
			{
				if ( identifier == -1 || fa->identifier == identifier )
				{
					if ( index == 0 )
						return fa;
					else
						index--;
				}
			}
		}
		if ( attributetype != -1 && fa->attributetype > attributetype ) break;
	}
	return NULL;
}

bool SMFTRecord::dofixup(int blocksize)
{
	return ::dofixup(this, recordlength_allocated, blocksize, fixuplistcount, (UINT16 *)( ((char *)this) + fixuplistoffset ) );
}

//-----------------------------------------------------------------------------

INT64 getxbytes(int count, const char *cp)
{
	if ( !cp || count < 1 || count > 8) return 0;

	INT64 result = 0;
	if ( cp[count-1] & 0x80 )
		result = 0xFFFFFFFFFFFFFFFF;
	memcpy(&result, cp, count);
	return result;
}

const char *NTFSfileruns::getfirstrun(fssize_t &offset, fssize_t &length, bool &sparse) const
{
	return getnextrun(offset, length, sparse, runs);
}

const char *NTFSfileruns::getnextrun(fssize_t &offset, fssize_t &length, bool &sparse, const char *prev) const
{
	if ( !prev || ! (*prev) ) return NULL;

	int lengthlen = ((*prev) & 0x0F);
	int offsetlen = ((*prev) & 0xF0) >> 4;
	sparse = offsetlen == 0;

	assert( offsetlen <= 8);

	prev++;

	length = getxbytes(lengthlen, prev);
	prev += lengthlen;
	offset = getxbytes(offsetlen, prev);
	prev += offsetlen;

	return prev;
}

//-----------------------------------

wstring SMFTAttribute::getname() const
{
	return wz2w((wchar_t *)(((char *)this)+nameoffset), namelength );
}

//--------------------------------------------------------------------------------


#define NTFSATTRIBUTELISTRECHEADERSIZE 6
ALArec *ALArec::read(CStream *f, void *buffer, int buffersize)
{
	if ( !f || !f->isvalid() ) return NULL;

	// alloc 6 bytes so we can read the type and size fields into memory
	ALArec *smalltemp = (ALArec *)alloca( NTFSATTRIBUTELISTRECHEADERSIZE );
	
	if ( f->Read(smalltemp, NTFSATTRIBUTELISTRECHEADERSIZE) != NTFSATTRIBUTELISTRECHEADERSIZE ) return NULL;
	if ( buffer && buffersize < smalltemp->recordlength ) return NULL;

	// malloc a full sized rec for the real data
	ALArec *rec = NULL;
	
	if ( buffer )
		rec = (ALArec *)buffer;
	else
		rec = (ALArec *)malloc( smalltemp->recordlength );
	if ( !rec ) return NULL;

	// copy the already read data into the new malloc'd rec
	rec->attributetype = smalltemp->attributetype;
	rec->recordlength = smalltemp->recordlength;

	int bytestoread = rec->recordlength - NTFSATTRIBUTELISTRECHEADERSIZE;

	// read the remainder of the data into the new rec
	if ( (f->Read(&rec->namelength, bytestoread) != bytestoread) || !rec->isvalid() )
	{
		if ( !buffer ) free(rec);	// only free rec if buffer == NULL
		return NULL;
	}
	return rec;
}

bool ALArec::isvalid() const
{
	return (recordlength >= sizeof(ALArec)) && (recordlength < MAXRECORDLENGTH);
}

bool ALArec::compare(const ALArec &rhs) const
{
	return  attributetype == rhs.attributetype &&
			identifier == rhs.identifier &&
			namelength == rhs.namelength &&
			getname() == rhs.getname();
}

wstring ALArec::getname() const
{
	wchar_t *temp = (wchar_t *)(((char *)this)+nameoffset);
	return wz2w(temp, namelength);
}

};		// end NTFS namespace
};		// end AccessData namespace

