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

/** @internal@file alt.cpp
*	@brief Implements all the <tt>ALT</tt>-related classes
*
*	This includes the implementation of all the supplied
*	guards, even the input guards (which are not defined in alt.h
*/

#include <vector>
#include "cppcsp.h"

using namespace csp;

bool csp::SkipGuard::enable(internal::AltingProcessPtr)
{
	//Skip guards are always ready
	return true;
}
bool csp::SkipGuard::disable(internal::AltingProcessPtr)
{
	//Skip guards are always ready
	return true;
}
void csp::SkipGuard::activate()
{
	//Skip guards have no effect
}

bool csp::RelTimeoutGuard::enable(internal::AltingProcessPtr proc)
{	
	CurrentTime(&timeout);
	timeout += time;

	timeoutId = addTimeoutAlt(&timeout,proc);
	
	Time cur;
	CurrentTime(&cur);
	
	
	return cur >= timeout;
}

bool csp::RelTimeoutGuard::disable(internal::AltingProcessPtr)
{
	removeTimeout(timeoutId);
	
	Time cur;
	CurrentTime(&cur);
	
	return cur >= timeout;
}

void csp::RelTimeoutGuard::activate()
{
	//Time-out guards have no effect
}

bool csp::TimeoutGuard::enable(internal::AltingProcessPtr proc)
{
	timeoutId = addTimeoutAlt(&time,proc);

	Time cur;
	CurrentTime(&cur);

	
	return cur >= time;
}

bool csp::TimeoutGuard::disable(internal::AltingProcessPtr)
{
	removeTimeout(timeoutId);
	
	Time cur;
	CurrentTime(&cur);
	
	return cur >= time;
}

void csp::TimeoutGuard::activate()
{
	//Time-out guards have no effect
}

csp::Alternative::Alternative(Guard** const guardArray,const unsigned int amount)
	:	favourite(0)
{
	//Copy the guard array:

	guards.resize(amount);
	for (unsigned int i = 0;i < amount;i++)
		guards[i] = guardArray[i];
}

csp::Alternative::~Alternative()
{
	//Delete all the guards:

	for (unsigned i = 0;i < guards.size();i++)
		delete guards[i];
}

unsigned int csp::Alternative::priSelect()
{
	signed int selected = ~0;
	signed int i;

	internal::AltingProcessPtr thisProcess( currentProcess() );

	altEnabling(thisProcess);	

	//check all the guards to see if any are ready already:

	for (i = 0;i < static_cast<signed int>(guards.size());i++)
	{		
		if (guards[i]->enable(thisProcess))
		{
			goto GOTO_FoundAnOption;
			//goto^n considered harmless for n = 1
		}
	}
	
	i -= 1;
	

	if (altShouldWait(thisProcess))
	{
		reschedule();	
	}

GOTO_FoundAnOption:
	//We can now assume that either i or somewhere before it contains 
	//a ready outputter
	
	for (;i >= 0;i--)
	{		
		if (guards[i]->disable(thisProcess))
		{
			selected = i;
		}
	}
	
	altFinish(thisProcess);

	//Now activate the selected guard:
	guards[selected]->activate();

	favourite = selected + 1;
	if (favourite >= static_cast<signed int>(guards.size())) 
		favourite -= static_cast<signed int>(guards.size());

	return static_cast<unsigned int>(selected);
}

unsigned int csp::Alternative::fairSelect()
{
	signed int selected = ~0;
	signed int i;

	internal::AltingProcessPtr thisProcess( currentProcess() );
	altEnabling(thisProcess);

	//check all the guards to see if any are ready already:

	for (i = favourite;i < static_cast<signed int>(guards.size());i++)
	{		
		if (guards[i]->enable(thisProcess))
		{
			goto GOTO_FoundAnOption;
			//goto^n considered harmless for n = 1
		}
	}
	
	for (i = 0;i < favourite;i++)
	{
		if (guards[i]->enable(thisProcess))
		{
			goto GOTO_FoundAnOption;
			//goto^n considered harmless for n = 1
		}
	}
	
	i -= 1;
	
	if (altShouldWait(thisProcess))
	{
		reschedule();	
	}

GOTO_FoundAnOption:
	//We can now assume that either i or somewhere before it contains 
	//a ready outputter
	

	if (i < favourite)
	{
		for (;i >= 0;i--)
		{		
			if (guards[i]->disable(thisProcess))
			{
				selected = i;
			}
		}
		i = static_cast<signed int>(guards.size()) - 1;
	}
	
	for (;i >= favourite;i--)
	{		
		if (guards[i]->disable(thisProcess))
		{
			selected = i;
		}
	}
	
	altFinish(thisProcess);

	//Now activate the selected guard:
	guards[selected]->activate();

	favourite = selected + 1;
	if (favourite >= static_cast<signed int>(guards.size())) 
		favourite -= static_cast<signed int>(guards.size());

	return static_cast<unsigned int>(selected);
}

unsigned int csp::Alternative::sameSelect()
{
	unsigned int n = fairSelect();
	
	if (--favourite < 0)
		favourite = static_cast<signed int>(guards.size()) - 1;

	return n;
}


Guard* csp::Alternative::replaceGuard(unsigned int index,Guard* guard)
{
	Guard* old;
	if (index >= guards.size())
	{
		return NULL;
	}

	old = guards[index];
	guards[index] = guard;
	return old;
}
