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

#ifndef BLOCKSTREAM_H
#define BLOCKSTREAM_H

#include "ADStream.h"
#include "ADIOBlockDevice.h"

#include <vector>

namespace AccessData
{

using std::vector;

// CBlockStream
// This is an readonly stream that is based on runs of blocks on a blockdevice.
class CBlockStream : public CStream
{
public:
	CBlockStream();
	CBlockStream(const CBlockStream &rhs);
	~CBlockStream();
	CBlockStream& operator=(const CBlockStream &rhs);

	bool				isvalid() const;
	void				clear();

	void				assign(const CBlockStream &rhs);

    // new stuff
	virtual INT64  		PhysicalLength() const;
	virtual int	   		BlockSize() const;
	virtual int			PhysicalBlockSize() const;

	//
	// This is how you build an CBlockStream:
	//
	void				SetDev(CFTKBlockDevice *adev);
	CFTKBlockDevice*	GetDev() const;

	INT64				BlockCount() const { return m_bc; }

	bool				AddRun(INT64 startblock, INT64 length);
    int					RunCount() const { return m_runs.size(); }
    bool				GetRunInfo(int runnum, INT64 &logicalstart, INT64 &physicalstart, INT64 &length);

	void				SetLength(INT64 asize);
	void				SetInitialOffset(int ainitialoffset);

	// mksubfile()
	// Makes f a subset of the current file.
	// (ie. mksubfile(f, 10, 100) would set f so that it was limited to the bytes from 10..109 in this file.
	bool				MakeSubFile(CBlockStream *f, INT64 pos, INT64 count);


	// Get the blocknumber (m_dev) that holds the byte at position pos.
	INT64				GetBlock(INT64 pos) const;
	INT64				GetBlock(INT64 pos, CFTKBlockDevice *dev) const;

	//
	// CStream inherited
	//
	CStream*			Dup() const;
    CBlockStream*		BlockStreamDup() const { CStream *s = Dup(); CBlockStream* x = dynamic_cast<CBlockStream*>(s); if (!x) delete s;  return x; }

	int					Read(void *dest, int bytestoread);
	int					Read(void *dest, int bytestoread, INT64 pos);

	bool				Eof();
	INT64				Seek(INT64 amount, SEEK_WHENCE whence);
	INT64				Tell() const;
	INT64				Length() const;
protected:
	struct runinfo
	{
		INT64	logicalstart;
		INT64	physicalstart;
		INT64	count;
	};
	typedef vector < runinfo > RUNINFOLIST;	// a list of runs of blocks where the file lives


	// xlat a file block number (via the runlist) into a block number on m_dev
	bool blocknumxlat(INT64 &physicalblocknum, INT64 logicalblocknum) const;

	CFTKBlockDevice*	m_dev;				// dev is a pointer to the device that stores the blocks for this file
	RUNINFOLIST			m_runs;				// a list of block runs that define this stream
	INT64				m_bc;				// blockcount: the number of blocks in the runlist
	INT64				m_size;				// the size of the stream in bytes
    int					m_blocksize;		// from dev->blocksize()
	int					m_initialoffset;	// the offset (in bytes) for the first block, to where the file really starts.
											// this should be less than dev->ftkbioBlockSizeGet().
	INT64				m_cp;				// cp is the current position in the file
private:
	typedef CStream inherited;
	void				initfields();
	void				clearfields();
	void				assignfields(const CBlockStream &rhs);
};

};

#endif
