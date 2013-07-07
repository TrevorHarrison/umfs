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


#include "Path.h"
#include <assert.h>

namespace AccessData
{


CPath::CPath()
{
	m_flags = 0;
}

CPath::CPath(const string &path)
{
	m_path = path;
	m_flags = getflags(m_path);
	fixtrailing(m_path, getsepchar(), false);
}

CPath::CPath(const CPath &rhs)
{
	m_path = rhs.m_path;
	m_flags = rhs.m_flags;
}

CPath::~CPath()
{
}

void CPath::clear()
{
	m_flags = 0;
	m_path = "";
}

CPath& CPath::operator=(const CPath &rhs)
{
	m_path = rhs.m_path;
	m_flags = rhs.m_flags;

	return *this;
}

CPath& CPath::operator=(const string &path)
{
	m_path = path;
	m_flags = getflags(m_path);
	fixtrailing(m_path, getsepchar(), false);

	return *this;
}

CPath& CPath::operator+=(const CPath &rhs)
{
	fixtrailing(m_path, getsepchar(), true);
	m_path += rhs.m_path;
	m_flags = getflags(m_path);

	return *this;
}

CPath& CPath::operator+=(const string &s)
{
	fixtrailing(m_path, getsepchar(), true);
	m_path += s;
	m_flags = getflags(m_path);
	fixtrailing(m_path, getsepchar(), false);

	return *this;
}

int CPath::compare(const CPath &rhs) const
{
	return pathstrcmp( m_path, rhs.m_path);
}

bool CPath::operator==(const CPath &rhs) const
{
	return pathstrcmp( m_path, rhs.m_path) == 0;		// this will only work if both paths have the same trailing seps... should force trailing seps before comparing
}

bool CPath::operator!=(const CPath &rhs) const
{
	return !operator==(rhs);
}

bool CPath::operator<(const CPath &rhs) const
{
	return pathstrcmp( m_path, rhs.m_path) < 0; // need to test this... I don't remember if it should be <0 or >0!!!!!
}

bool CPath::is(int testflags) const
{
	return (m_flags & testflags) != 0;
}

bool CPath::issamevolume(const CPath &rhs) const
{
	if ( m_flags != rhs.m_flags ) return false;

	if ( is( PATH_UNC ) )
	{
		return pathstrcmp(getpart(0), rhs.getpart(0)) == 0 && pathstrcmp( getpart(1), rhs.getpart(1)) == 0;
	} else
	if ( is( PATH_WIN ) )
	{
		string d1 = getpart(0);
		string d2 = rhs.getpart(0);

		return stricmp( d1.c_str(), d2.c_str() ) == 0;
	}

	return false;
}

bool CPath::ischild(const CPath &rhs) const
{
	if ( m_flags != rhs.m_flags ) return false;
	if ( m_path.length() > rhs.m_path.length() ) return false;	// quick hack

	CPath temp = rhs.sub(0, size() );		// get a subpath of rhs that matches the number of components in this

	return compare(temp) == 0;
}

string CPath::str(bool trailingsep) const
{
	string s = m_path;
	fixtrailing(s, getsepchar(), trailingsep);
	return s;
}

int CPath::size() const
{
	int count = 0;
	unsigned int start = 0, len = 0;
	while ( getpartinfo(count, start, len, start, count) )
	{
		count++;
		start += len;
	}
	return count;
}

string CPath::operator[](int index) const
{
	return getpart(index);
}

string CPath::getpart(int index) const
{
	unsigned int start, len;
	string s;

	if ( index < 0 )
	{
		int count = size();
		index = count + index;
	}

	if ( getpartinfo(index, start, len) ) s = m_path.substr(start, len);
	return s;
}

CPath CPath::sub(int start, int len) const
{
	if ( len == 0 ) return CPath();

	unsigned int pstart1, plen1;
	unsigned int pstart2, plen2;

	if ( start < 0 )
	{
		int count = size();
		start = count + start;
	}

	if ( start == 0 ) 
	{
		pstart1 = 0;
	} else
	{
		if ( !getpartinfo(start, pstart1, plen1) ) return CPath();
	}

	if ( len < 0 || !getpartinfo(start+len-1, pstart2, plen2) ) { pstart2 = m_path.length(); plen2 = 0; }

	return CPath( m_path.substr( pstart1, pstart2+plen2-pstart1) );
}

CPath CPath::getpath() const
{
	int xsize = size();
	return sub(0, xsize-1);
}

void CPath::explode( std::vector<string> &dest ) const
{
	dest.clear();

	unsigned int start = 0, len, partnum = 0;
	while ( getpartinfo(partnum, start, len, start, partnum) )
	{
		dest.push_back( m_path.substr( start, len ) );
		partnum++;
		start += len;
	}
}

void CPath::replace(int index, const string &s)
{
	if ( index < 0 )
	{
		int count = size();
		index = count + index;
	}

	unsigned int start, len;
	if ( !getpartinfo(index, start, len) ) return;

	m_path.replace(start, len, s);
}

void CPath::remove(int index, unsigned int count)
{
	if ( count == 0 ) return;

	if ( index < 0 )
	{
		int xcount = size();
		index = xcount + index;
	}

	unsigned int start1, len1;
	unsigned int start2, len2;

	if ( !getpartinfo(index, start1, len1) ) return;
	if ( !getpartinfo(index+count, start2, len2) ) start2 = m_path.length();

	m_path.erase(start1, start2-start1);

	//update the flags here?
}

string CPath::getext() const
{
	string s = operator[](-1);
	unsigned int dot = s.rfind('.');
	if ( dot == string::npos ) return string();
	return s.substr(dot+1);
}

void CPath::setext(const string &ext)
{
	string s = getpart(-1);
	unsigned int dot = s.rfind('.');
	if ( dot == string::npos )
	{
		s += ".";
		s += ext;
	} else
	{
		s.replace(dot+1, string::npos, ext);
	}

	replace(-1, s);
}

void CPath::trimext()
{
	string s = getpart(-1);
	unsigned int dot = s.rfind('.');
	if ( dot == string::npos ) return;

	s.erase(dot);

	replace(-1, s);
}

char CPath::getsepchar() const
{
	string s = getsep();
	return s.length() ? s[0] : '\0';
}

string CPath::getsep(int flags) const
{
	switch ( flags & PATH_PATH )
	{
		default:
		case PATH_UNC:		
		case PATH_WIN:		return "\\"; break;
		case PATH_URL:
		case PATH_UNIX:		return "/"; break;
		case PATH_MAC:		return ":"; break;
	}
}

// Supported path types:
// [path-with-trailing-path-seperator] [filename.ext]
//
// DOS/WIN32:
// UNC: \\server\vol\dir\dir\  blah
// Normal: c:\dir\dir\   blah
//
// Unix:
// /usr/local/bin/    lah
//
// Mac:
// :something:something:wild:ass:guess
//
// URL:
// protocol://server/path/path/   blah

#define MAXURLPROTOCOLLEN 10
int CPath::getflags(const string &path) const
{
	int pathsize = path.size();
	if ( pathsize == 0 ) return 0;

	unsigned int colon = path.find(':');
	unsigned int bslash = path.find('\\');
	unsigned int bslash2 = bslash != string::npos ? path.find('\\', bslash+1) : string::npos;

	int flags = 0;
	if ( bslash == 0 && bslash2 == 1 ) 
	{
		if ( pathsize > 4 )
		{
			flags |= PATH_WIN | PATH_UNC;
			flags |= path[2] == '.' ? PATH_REL : PATH_FQ;
		}
	} else
	if ( colon == 1  )
	{
		flags |= PATH_WIN;
		flags |= ( bslash == 2 ) ? PATH_FQ : PATH_REL;
	} else
	if ( bslash != string::npos )
	{
			flags |= PATH_WIN | PATH_REL;
	}

	//string sep = getsep(flags);
	//if ( !sep.length() ) return flags;

//	if ( pathsize > 0 && path[pathsize - 1] != sep[0] )
//	{
//		flags |= PATH_FILE;
//	}

	return flags;
#if 0
	int slash = path.find('/');
	int slash2 = slash != string::npos ? path.find('/', slash+1) : string::npos;
	else
	if ( colon == 0 )
	{
		flags |= PATH_MAC;
	} else
	if ( colon < MAXURLPROTOCOLLEN && colon == slash-1 && slash == slash2-1 ) 
	{
		flags |= PATH_URL;
	} else
	if ( 0 < slash && slash < colon && slash2 == string::npos ) 
	{
		flags |= PATH_NETWARE;
	} else
	if ( slash != string::npos ) 
	{
		flags |= PATH_UNIX; // return pntUNIX;
	} else
	if ( bslash == 0 && blash2 == 1 ) 
	{
		flags |= PATH_UNC;
	} else
	if ( colon != string::npos && colon != 1 ) 
	{
		//return pntUnknown;
	}
#endif
}

bool CPath::getpartinfo(unsigned int index, unsigned int &start, unsigned int &len, unsigned int starthint, unsigned int partnumhint) const
{
	start = starthint;
	len = 0;
	unsigned int curpart = partnumhint;

	unsigned int pathend = m_path.length();

	if ( curpart == 0 )
	{
		switch ( m_flags & PATH_PATH )
		{
			case PATH_WIN:
			{
				if ( is( PATH_UNC ) )
				{
					assert( pathend > 4 && m_path[0] == '\\' && m_path[1] == '\\' );
					start = 2;
					break;
				} else
				{
					if ( is( PATH_FQ ) )
					{
						assert( pathend > 1 && ( is(PATH_FQ) && m_path[1] == ':') );
						if ( index == 0 )
						{
							start = 0;
							len = 2;
							return true;
						}

						start = 2;
						curpart = 1;
					}
				}
			}
			break;
		}
	}

	string sepstr = getsep();
	char sepchar = sepstr[0];
	if ( m_path[start] == sepchar ) start++;

	while ( start < pathend )
	{
		unsigned int sep = m_path.find(sepchar, start);
		if ( sep == string::npos ) sep = pathend;
		if ( curpart == index )
		{
			len = sep-start;
			return true;
		}
		start = sep+1;
		curpart++;
	}

	return false;
}

void CPath::fixtrailing(string &s, char sep, bool b) const
{
	if ( !s.length() ) return;

	bool hasts = s[ s.length() - 1] == sep;
	if ( b && !hasts) s += sep; else
	if ( !b && hasts ) s.erase( s.length() - 1);
}

inline int CPath::pathstrcmp( const string &p1, const string &p2) const
{
	return is( PATH_CASESENS ) ? strcmp(p1.c_str(), p2.c_str()) : stricmp( p1.c_str(), p2.c_str());
}

};	// end of namespace