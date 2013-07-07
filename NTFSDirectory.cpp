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

#include "NTFSDirectory.h"

#include "NTFSCommon.h"
#include "ADIOString.h"

#include <malloc.h>

namespace AccessData
{
namespace NTFS
{

void CNTFSDirectory::initfields()
{
	m_ntfs = NULL;
	m_currententry = 0;
}

void CNTFSDirectory::clearfields()
{
	m_ntfs = NULL;
    m_file.clear();
	m_ufidlist.clear();
	m_currententry = 0;
}

void CNTFSDirectory::assignfields(const CNTFSDirectory &rhs)
{
	m_ntfs = rhs.m_ntfs;
    m_file = rhs.m_file;
    m_mftrecnum = rhs.m_mftrecnum;
	m_ufidlist = rhs.m_ufidlist;
	m_currententry = rhs.m_currententry;
}


CNTFSDirectory::CNTFSDirectory()
{
	initfields();
}

CNTFSDirectory::CNTFSDirectory(const CNTFSDirectory &rhs) : inherited(rhs)
{
	initfields();
	assignfields(rhs);
}

CNTFSDirectory::~CNTFSDirectory()
{
	clearfields();
}


bool CNTFSDirectory::isvalid() const
{
	return m_ntfs != NULL;
}

void CNTFSDirectory::clear()
{
	clearfields();
}

bool CNTFSDirectory::open(CNTFS *ntfs, MFT_RECNUM mftrecnum /*CFTKNTFSFile *afile*/)
{
	clear();
	if ( !ntfs ) return false;

	m_ntfs = ntfs;
	m_mftrecnum = mftrecnum;

	if ( !m_file.open(ntfs, &ntfs->m_mft, mftrecnum, 0, false) ) { clear(); return false; }

	m_file.getdirattribnums(m_ir_attribnum, m_ia_attribnum, m_bm_attribnum);
    if ( m_ir_attribnum == 0xFFFF ) { clear(); return false; }

	return true;
}

bool CNTFSDirectory::read(CNTFS *ntfs, CStream *indexnodestream, NTFSindexentrylist *indexentrylist, int indexnodesize, int blocksize, const wstring &name, int attribs)
{
	if ( !ntfs || !indexentrylist ) return false;

	NTFSindexentry *ie = indexentrylist->getfirstentry();
	while ( ie )
	{
		if ( ie->issubnode() )
		{
			if ( !indexnodestream ) return false;

			NTFSindexnode *indexnode = NTFSindexnode::Read(indexnodestream, ie->getsubnodeblock() & 0xFFFFFF, indexnodesize, blocksize);
			if ( !indexnode ) return false;

			bool result = read(ntfs, indexnodestream, &indexnode->indexentries, indexnodesize, blocksize, name, attribs);
			free(indexnode);
			if ( !result ) return false;
		}
		if ( !ie->islast() )
		{
			SFilenameAttrib &fna = ie->getfilenameattribute();
			if ( m_mftrecnum.RecNum() != ie->fileref.RecNum() && fna.filenamespace != 2 && ie->fileref.RecNum() != sfrBadClusters )	// don't get self refs and ignore DOS names
			{
				wstring filename = fna.getfilename();

				bool match = true;
				if ( name.length() != 0 )
				{
					if ( AD_WCSICMP(filename.c_str(), name.c_str()) != 0 ) match = false;
				}

				if ( match )
				{
					//CFTKFileRef tempfileref( ntfs2ufid(0, ie->fileref.RecNum(), false) );
                    UINT64 mftrecnum = ie->fileref.RecNum();
					CNTFSFile *f = ntfs->openfile(mftrecnum, 0, false);
					if ( f )
					{
                    	vector<UINT16> attribnums;
                        if ( (attribs & dsaFILE) == dsaFILE )
                        {
							f->getdataattribnums( attribnums );
                            for(unsigned int i = 0; i < attribnums.size(); i++)
                            {
                            	m_ufidlist.push_back( ntfs2ufid(attribnums[i], mftrecnum, false));
                            }
                        }
                        if ( (attribs & dsaDIRECTORY) == dsaDIRECTORY )
                        {
                        	f->getdirattribnums( attribnums );
                            for(unsigned int i = 0; i < attribnums.size(); i++)
                            {
                            	m_ufidlist.push_back( ntfs2ufid(attribnums[i], mftrecnum, false));
                            }
                        }
						delete f;
					}
				}
			}
		}
		ie = indexentrylist->getnextentry(ie);
	}
	return true;
}

int CNTFSDirectory::getslackspace()
{
	int slackspace = 0;

	CBlockStream *rootstream = m_file.openstream(m_ir_attribnum, false);
	CBlockStream *bmstream = m_file.openstream(m_bm_attribnum, false);
	CBlockStream *indexnodestream = m_file.openstream(m_ia_attribnum, false);
	NTFSindexroot *indexroot = NTFSindexroot::Read(rootstream);
    if ( rootstream && indexroot && bmstream && indexnodestream )
    {
		int blocksize = indexnodestream->PhysicalBlockSize();
        int indexnodesize = indexroot->indexnodesize;
        int nodecount = indexnodestream->Length() / indexnodesize;

    	CNTFSBitmap bm;
        bm.open(bmstream, nodecount);
        bmstream = NULL;

        bool bitvalue;
        for(INT64 bitnum = 0, bitcount = bm.count(); bitnum < bitcount; bitnum++)
        {
        	if ( bm[bitnum] )
            {
				NTFSindexnode *indexnode = NTFSindexnode::Read(indexnodestream, bitnum, indexnodesize, blocksize);
                if ( indexnode )
                {
                	slackspace += indexnode->indexentries.listsize - indexnode->indexentries.listend;
	                free(indexnode);
                }
            } else
            {
            	slackspace +=indexnodesize - 0x58;	// empty index node could have about this much free
            }
        }

    }

    delete indexroot;
    delete rootstream;
    delete bmstream;
    delete indexnodestream;

    return slackspace;
}

CFile *CNTFSDirectory::FindFirst(const wstring &name, int attribs)
{
	if ( !isvalid() ) return NULL;

	// Reset
	FindReset();

	CStream *rootstream = m_file.openstream(m_ir_attribnum, false);
	if ( !rootstream ) return NULL;
	NTFSindexroot *indexroot = NTFSindexroot::Read(rootstream);
	delete rootstream;
	if ( !indexroot ) return NULL;

	CBlockStream *indexnodestream = NULL;
	int blocksize = 0;

	if ( indexroot->islargeindex() )
	{
		indexnodestream = m_file.openstream(m_ia_attribnum, false);
		if ( !indexnodestream ) { free(indexroot); return NULL; }
		blocksize = indexnodestream->PhysicalBlockSize();
	}

	bool result = read(m_ntfs, indexnodestream, &indexroot->indexentries, indexroot->indexnodesize, blocksize, name, attribs);

	free(indexroot);
	if ( indexnodestream )
	{
		delete indexnodestream;
		indexnodestream = NULL;
	}

	return result ? FindNext(name, attribs) : NULL;
}

CFile *CNTFSDirectory::FindNext(const wstring &name, int attribs)
{
	if ( !isvalid() ) return NULL;

	while ( m_currententry < m_ufidlist.size() )
	{
		UFID_t temp = m_ufidlist[m_currententry];
		m_currententry++;
		CFile *file = m_ntfs->OpenFile(temp);
		if ( file ) return file;
	}
	return NULL;
}

void CNTFSDirectory::FindReset()
{
	if ( isvalid() )
	{
		m_ufidlist.clear();
		m_currententry = 0;
	}
}

}		// end namespace NTFS
}		// end namespace AccessData
