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

#include "NTFSindexstructs.h"
#include "NTFScommon.h"


namespace AccessData
{
namespace NTFS
{

//---------------------------------------------------------------------------
bool NTFSindexentry::isvalid()
{
	return datalength < reclength;
}

INT64 NTFSindexentry::getsubnodeblock()
{
	return *(INT64 *)( ((char *)this)+reclength-8 );	// the subnodevcn is store 8 bytes from end of record
}


//---------------------------------------------------------------------------

bool NTFSindexentrylist::isvalid()
{
	return listend < listsize;
}

NTFSindexentry *NTFSindexentrylist::getfirstentry()
{
	if ( liststart == 0 ) return NULL;

	NTFSindexentry *ie = (NTFSindexentry *)(((char *)this)+liststart);
	return ie;
}

NTFSindexentry *NTFSindexentrylist::getnextentry(NTFSindexentry *prev)
{
	if ( !prev || prev->islast() ) return NULL;

	NTFSindexentry *ie = (NTFSindexentry *)(((char *)prev)+prev->reclength);
	return ie;
}

NTFSindexentry *NTFSindexentrylist::findentry(const wchar_t *name, bool &issubnode)
{
	issubnode = false;

	for(NTFSindexentry *ie = getfirstentry(); ie != NULL; ie = getnextentry(ie) )
	{
		if ( ie->islast() )
		{
			if ( ie->issubnode() )
			{
				issubnode = true;
				return ie;
			} 
			break;
		}
		int cmp = wcscmp(ie->getfilenameattribute().getfilename().c_str(), name);

		if ( cmp == 0 ) return ie;

		if ( cmp > 0 && ie->issubnode() )
		{
			issubnode = true;
			return ie;
		} 
 	}
	return NULL;
}

//---------------------------------------------------------------------------

bool NTFSindexroot::isvalid()
{
	return attributetype == atFILENAME && indexentries.isvalid();
}

bool NTFSindexroot::islargeindex()
{
	return flags & 1;
}

#define MAXINDEXROOTSIZE 10000
NTFSindexroot *NTFSindexroot::Read(CStream *f)
{
	if ( !f || !f->isvalid() ) return NULL;

	int indexrootsize = f->Length();
	if ( indexrootsize > MAXINDEXROOTSIZE ) return NULL;

	NTFSindexroot *temp = (NTFSindexroot *)malloc( indexrootsize );
	if ( !temp ) return NULL;

	if ( f->Read(temp, indexrootsize, 0 ) != indexrootsize && !temp->isvalid() )
	{
		free(temp);
		return NULL;
	}
	return temp;
}

//---------------------------------------------------------------------------

bool NTFSindexnode::isvalid()
{
	return indexsig == INDEXRECSIG && indexentries.isvalid();
}

bool NTFSindexnode::dofixup(int mysize, int blocksize)
{
	return ::dofixup(this, mysize, blocksize, fixuplistcount, (UINT16 *)( ((char *)this) + fixuplistoffset ) );
	return true;
}

NTFSindexnode *NTFSindexnode::Read(CStream *f, INT64 nodenum, int indexnodesize, int blocksize)
{
	if ( !f || !indexnodesize || !blocksize ) return NULL;

	NTFSindexnode *temp = (NTFSindexnode *)malloc( indexnodesize );
	if ( !temp ) return NULL;

	if ( (f->Read(temp, indexnodesize, nodenum * indexnodesize) != indexnodesize) || !temp->isvalid() || !temp->dofixup(indexnodesize, blocksize) )
	{
		free(temp);
		return NULL;
	}
	return temp;
}


}		// end namespace NTFS
}		// end namespace AccessData
