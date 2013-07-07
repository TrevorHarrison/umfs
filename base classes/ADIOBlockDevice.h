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

#ifndef FTKBLOCKDEVICE_H
#define FTKBLOCKDEVICE_H

#include "IntTypes.h"
#include "JobCallBack.h"
#include "ADIOBaseMD.h"

namespace AccessData
{

// CFTKBlockDevice
// This class defines the interface into a 'block' device, ie. a device
// that can be divided into randomly accessable fixed length chunks.
// Classes that derive from this interface must provide implementations
// for all methods.  Use CFTKBDBase for a parent class that provides some
// default behaviour.
class CFTKBlockDevice : public CFTKBaseMD
{
public:
	// ftkbioBlockRead()
	// Reads block blocknum into dest.  Use ftkbioBlockSize() to insure that
	// dest is big enough to handle the block.
	// Returns bool indicating success/failure (IO error)
	virtual bool			ftkbioBlockRead(void *dest, fssize_t blocknum, int startoffset=0, int bytestoread=-1) = 0;

	// ftkbioBlockReadN()
	// Reads sequential blocks into dest, starting at startblocknum and going for count blocks.
	// Returns -1 on error or number of blocks read on success.
	virtual int				ftkbioBlockReadN(void *dest, fssize_t startblocknum, int count) = 0;

	// ftkbioBlockSizeGet()
	// Returns the size of blocks on this device.
	virtual int				ftkbioBlockSize() const = 0;

	// ftkbioPhysicalBlockSize()
	// Returns the physical size of blocks on this device
	virtual int				ftkbioPhysicalBlockSize() const = 0;

	// ftkbioFirstBlockNumGet()
	// Returns the first valid block number on this device
	virtual fssize_t		ftkbioFirstBlockNum() const = 0;

	// ftkbioBlockCountGet()
	// Returns the number of blocks on this device (ie. lastblocknum+1)
	virtual fssize_t		ftkbioBlockCount() const = 0;

	// ftkbioIsPhysicalDevice()
	// Returns true if this block device is a physical device and not a logical volume
	virtual bool			ftkbioIsPhysicalDevice() const = 0;

	// ftkbioBlockNumTranslate()
	// Translates blocknum (which is relative to this instance) to a blocknum that is relative to the device dev.
	virtual fssize_t		ftkbioBlockNumTranslate(fssize_t blocknum, CFTKBlockDevice *dev) const = 0;

	// ftkbioVerify()
	// Verifies any checksums or hashes built into the block device to check for errors.
	// This is likely to take a very long time as all the blocks on the device need to be read and have
	// some checksum or hash function run on them.
	// Returns VERIFY_NOT_SUPPORTED if this block device doesn't support this operation.
	// Updates &curblocknum with its progress as it is verifying the blockdevice.  (this might be problematic since its a int64 and might not be updated atomicly)
	enum EVerifyResult
	{
		VERIFY_OK,
		VERIFY_FAILED,
		VERIFY_NOT_SUPPORTED
	};
	virtual EVerifyResult	ftkbioVerify(CJobCallBack &callback) = 0;
	virtual	bool			ftkbioVerifySupported() = 0;


	// Flush any cached data so that the next read gets fresh data off the disk
	virtual void			ftkbioFlush() = 0;
};

};		// end namespace

#endif
