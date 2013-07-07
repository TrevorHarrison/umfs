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

#ifndef PATH_H
#define PATH_H

// The following pragma disables the warning about truncated debug info.
#pragma warning( disable : 4786 )

#include "StringTypes.h"
#include <vector>

namespace AccessData
{

enum EPathFlags
{
	PATH_WILD		= 0x00000008,
	PATH_WIN		= 0x00001000,
	PATH_UNIX		= 0x00002000,
	PATH_MAC		= 0x00004000,
	PATH_URL		= 0x00008000,
	PATH_PATH		= 0x0000F000,		// mask for any path
	PATH_UNC		= 0x00010000,		// added to PATH_WIN if its a unc path
	PATH_FQ			= 0x00020000,		// this is a fully qualified path
	PATH_REL		= 0x00040000,		// this is a relative path
	PATH_CASESENS	= 0x0000E000,		// unix+mac+url
};

// CPath
// Class to represent and manipulate a string path.
class CPath
{
public:
	CPath();
	CPath(const string &path);
	CPath(const CPath &rhs);
	~CPath();

	void	clear();

	// assignment
	CPath&	operator=(const CPath &rhs);
	CPath&	operator=(const string &path);

	// Append a path to the end of this path, subject to some sanity checks
	CPath&	operator+=(const CPath &rhs);
	CPath&	operator+=(const string &s);

	// Test this path with another path
	int		compare(const CPath &rhs) const;
	bool	operator==(const CPath &rhs) const;
	bool	operator!=(const CPath &rhs) const;
	bool	operator<(const CPath &rhs) const;

	// Test the path for certain attributes
	bool	is(int testflags) const;
	int		getflags() const { return m_flags; }
	bool	isblank() const { return m_path.length() == 0; }

	// Test volume equiv, useful for optimizing a file or directory move
	bool	issamevolume(const CPath &rhs) const;

	// Test if rhs is a child path of *this, useful in detecting recursive copytree operations
	bool	ischild(const CPath &rhs) const;

	// Get the full path, add a trailing seperator (ie. '\\') if requested.
	string	str(bool trailingsep=false) const;

	// The number of components in the path
	int		size() const;

	// Get a sub part of the path.  Negative indexes get items from right
	// to left (ie. -1 == last component of the path, -2 is second to last component, etc)
	string	operator[](int index) const;
	string	getpart(int index) const;
	CPath	sub(int start=0, int len=-1) const;

	// Get the filename of the path (ie. the last component of the path, same as getapart(-1) )
	string	getfilename() const				{ return getpart(-1); }
    void	setfilename(const string &s)	{ replace(-1, s); }

	// Get everything but the last component of the path.
	CPath	getpath() const;

	// Breaks the path into component parts and puts them in dest
	void	explode( std::vector<string> &dest ) const;

	// Replace a component of the path with s
	void	replace(int index, const string &s);

	// Remove a part of the path
	void	remove(int index, unsigned int count);

	// Get/set the extension of the last component of the path
	string	getext() const;
	void	setext(const string &ext);
	void	trimext();

	// Get the native seperator char (ie. "\\", or "/", or ":"
	// This is mostly used by the private methods, but may be useful to public users.
	// Some path types might allow multiple seperators.
	char	getsepchar() const;
	string	getsep() const					{ return getsep(m_flags); }
	string	getsep(int flags) const;

protected:
	string	m_path;
	int		m_flags;

	int		getflags(const string &path) const;
	bool	getpartinfo(unsigned int index, unsigned int &start, unsigned int &len, unsigned int starthint=0, unsigned int partnumhint=0) const;
	void	fixtrailing(string &s, char sep, bool b) const;
	int		pathstrcmp( const string &p1, const string &p2) const;

};


}		// end namespace AccessData

#endif