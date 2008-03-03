/*
*	The Kent C++CSP Library 
*	Copyright (C) 2002-2007 Neil Brown
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../src/cppcsp.h"

extern bool runTests(bool incPerfTests);


#ifdef CPPCSP_WINDOWS
	int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
	{		
		return (runTests(false) ? 0 : 1);
	}
#else
	int main(int,char**)
	{
		return (runTests(false) ? 0 : 1);
	}
#endif
