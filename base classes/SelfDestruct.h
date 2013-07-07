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

#ifndef SELFDESTRUCT_H
#define SELFDESTRUCT_H

namespace AccessData
{

// Class used to insure resetting of your object due to errors or
// incorrect usage in an initialization method (ie. some kind of open(...) )
// Your class must have a .clear() method.  In the method you wish to
// protect, instansiate an local copy of a CSelfDestruct<CYourClassType>.
// When you successfully complete your method, call selfdistruct.disarm():
// bool CMyClass::foo()
// {
//		CSelfDestruct<CMyClass> selfdestruct(this);
//		...dowork...
//		if ( someerror ) return false;		// early return causes selfdestruct to trigger
//		..domorework...
//		selfdestruct.disarm();				// function was successful
//		return true;
// }
template<class C>
class CSelfDestruct
{
public:
	CSelfDestruct( C* c ) : m_c( c ), m_ok( false ) { }
	~CSelfDestruct()	{ if( !m_ok ) m_c->clear(); }
	void	disarm()	{ m_ok = true; }
private:
	C*		m_c;
	bool	m_ok;

	CSelfDestruct();	// disallow
	CSelfDestruct( const CSelfDestruct<C>& rhs );					// disallow
	CSelfDestruct<C> &operator =( const CSelfDestruct<C>& rhs );	// disallow
};

};

#endif
