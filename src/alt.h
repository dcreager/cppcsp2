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

#include <vector>
#include <list>

namespace csp
{
	class Alternative;
	
	/**
	*	An <tt>ALT</tt> guard.
	*
	*	A guard in an Alternative represents one possible choice for the Alternative to select.
	*	There are three different types of guards supplied with C++CSP2:
	*	- Channel guards; use methods such as AltChanin::inputGuard()
	*	- Timeout guards; RelTimeoutGuard and TimeoutGuard
	*	- The skip guard, SkipGuard
	*
	*	You should refer to the documentation for the specific guards to see what they are used for.
	*
	*	@see Alternative
	*/
	class Guard
	{
	protected:
		/**@internal
		*	Enables the guard.
		*
		*	@return whether the guard is ready now
		*/
		virtual bool enable(internal::AltingProcessPtr) = 0;

		/**@internal
		*	Disables the (previously enabled) guard.
		*
		*	@return whether the guard is ready now
		*/
		virtual bool disable(internal::AltingProcessPtr) = 0;		

		/**@internal
		*	Activates the guard.
		*
		*	This has no effect for skip guards and timeout guards
		*	but for channel guards it performs the communication
		*/
		virtual void activate() {}

	public:
		///@internal Empty virtual destructor
		virtual ~Guard() {};

		/**@internal
		*	Alternative is a friend because:
		*
		*	It needs to call our functions
		*/
		friend class Alternative;
	};

	/**
	*	An <tt>ALT</tt> relative timeout guard, for use in an Alternative.
	*
	*	This guard allows you to specify a timeout relative to when the Alternative begins its select call.
	*
	*	This is the most common form of timeout guard as
	*	it allows you to set a time to wait for the other guards
	*	in the <tt>ALT</tt> before timing out.
	*
	*	@see Alternative
	*	@ingroup time
	*/
	class RelTimeoutGuard : public Guard, private internal::Primitive
	{
	private:
		/**
		*	The length of time to wait from the start of the
		*	<tt>ALT</tt> before firing the guard
		*/
		csp::Time time;
		
		///The timeout value for the current ALT
		csp::Time timeout;

		/**
		*	The Id of the timeout we are waiting for
		*/
		internal::TimeoutId timeoutId;
				
	protected:
		virtual bool enable(internal::AltingProcessPtr);
		virtual bool disable(internal::AltingProcessPtr);
		virtual void activate();
	public:
		/**
		*	<tt>ALT</tt> relative timeout guard constructor
		*
		*	@param t The time to wait for from the start of the
		*	<tt>ALT</tt> before triggering the guard
		*/
		inline RelTimeoutGuard(const Time& t)
			:	time(t)
		{			
		};		
	};

	/**
	*	An <tt>ALT</tt> absolute timeout guard, for use in an Alternative.
	*
	*	This guard allows you to specify an absolute time at which the guard will become ready.
	*
	*	@see Alternative
	*	@ingroup time
	*/
	class TimeoutGuard : public Guard, private internal::Primitive
	{
	private:
		/**
		*	The absolute time at which to timeout
		*/
		csp::Time time;
		/**
		*	The Id of the timeout we are waiting for
		*/
		internal::TimeoutId timeoutId;
				
	protected:
		virtual bool enable(internal::AltingProcessPtr);
		virtual bool disable(internal::AltingProcessPtr);
		virtual void activate();		
	public:
		/**
		*	<tt>ALT</tt> timeout guard constructor
		*
		*	@param t The absolute time to at which to trigger the guard
		*/
		inline TimeoutGuard(const Time& t)
			:	time(t)
		{
		};		
	};

	/**
	*	An <tt>ALT</tt> skip guard, for use in an Alternative.
	*
	*	A skip guard is always ready, and is only useful at the bottom
	*	of a <tt>PRI ALT</tt> (Alternative::priSelect()) to ensure that the <tt>ALT</tt> never blocks.
	*	Using this guard with Alternative::fairSelect() will almost certainly 
	*	produce useless behaviour - the skip guard would sometimes be first
	*	in the order, meaning that none of the other guards would be checked.
	*	
	*	Equivalent to occam's <tt>SKIP</tt> guard.
	*
	*	@see Alternative
	*/
	class SkipGuard : public Guard
	{
	protected:
		virtual bool enable(internal::AltingProcessPtr);
		virtual bool disable(internal::AltingProcessPtr);
		virtual void activate();		
	};

	/**
	*	A class for performing <tt>ALT</tt>s.  This class is featured in the @ref tut-alt "Alternative" section in the guide.
	*
	*	For those familiar with occam: this is a class for performing <tt>ALT</tt>s over a set of guards.
	*
	*	For those not familiar with occam: an ALT is a way for a process
	*	to select between multiple externally-originating events.
	*
	*	The most common use for an Alternative is to wait for input on a number
	*	of channels, but to only actually communicate on one of the channels.
	*	For example, a process might want to wait for input on either its
	*	data channel, or on a "control signal" channel, giving priority to the
	*	control signal.  This could be achieved as follows:
	*	@code
			//dataIn is of type AltChanin<int>
			//controlIn is of type AltChanin<bool>
			//The boost::assign namespace is in scope, and <boost/assign/list_of.hpp> has been included
			
			bool signal;
			int data;
			
			Alternative alt( list_of<Guard*>(controlIn.inputGuard())(dataIn.inputGuard()) );
			
			while (true)
			{
				switch (alt.priSelect())
				{
					case 0: //control signal:
						controlIn >> signal;
						... Process the signal ...
						break;
					case 1: //data
						dataIn >> data;
						... Process the data ...
						break;
				}
			}
		@endcode
	*	
	*	A channel guard (returned by AltChanin::inputGuard) is ready 
	*	either when data is ready to be read from a channel, or when the channel is poisoned.  If the channel
	*	is poisoned, and the guard for that channel is not selected (because a higher priority guard was ready)
	*	nothing special happens.  If the guard for a poisoned channel is selected, fairSelect()/priSelect()/sameSelect()
	*	will return the index of the channel, and when you then try to read from the channel, a PoisonException will
	*	be thrown.
	*
	*	If you want to perform an extended input from the channel you are ALTing on, use the guard returned by AltChanin::inputGuard()
	*	as normal, but then in the body of the case statement below the ALT, perform an extended input instead of a normal input.
	*
	*	It is the user's responsibility to perform the input after the appropriate guard is selected.  If you do not
	*	perform an input (or begin an extended input) as your first action (specifically, as your first action before performing any other channel
	*	communications or barrier synchronizations) then undefined behaviour may result, depending on the channel type
	*	involved.
	*	
	*	Alternative is not usually a one-use class.  It can best be
	*	thought of as a container for an array of guards that can then
	*	perform a fair <tt>ALT</tt> or <tt>PRI ALT</tt> on these guards
	*	any number of times (even switching between fairSelect and priSelect as you wish).
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	In C++CSP v1.x, inputGuard() (and extInputGuard) took a parameter specifying where to store the data, and then the
	*	Alternative performed the input for you if that guard was selected.  This is no longer the case, as evidenced
	*	by the change in the signature of inputGuard() (which will help you notice the problem in old code).  Instead,
	*	the JCSP approach has been adopted, and you perform the input explicitly after the select method returns.  This
	*	is no slower, is much easier to perform extended inputs (with the new ScopedExtInput class), simpler to deal with
	*	poison and allows you to be more flexible in where you store the result of each input, in exchange for putting a 
	*	responsibility onto the user to remember to perform the input.
	*
	*	In C++CSP v1.x, the behaviour of an Alternative when a channel was poisoned was not explicitly
	*	defined, and could appear inconsistent in practice.  This has now been explicitly defined above.
	*	
	*	@see Guard
	*/
	class Alternative : private internal::Primitive
	{
	private:
		/**
		*	The guard array
		*/
		std::vector<Guard*> guards;

		/**
		*	Favourite index for fair <tt>ALT</tt>s
		*/
		signed int favourite;

	public:
		/** 
		*	Constructor, taking the guard array
		*
		*	Note that this function does not actually take an array of
		*	guards as it needs to take advantage of inheritance from guards.
		*	Therefore it takes an array of pointers to guards.
		*
		*	Alternative copies guardArray so it ends up with its own copy
		*	of the array of pointers (that is <i>not</i> guardArray) but
		*	the pointers point to the <i>same</i> guards as guardArray.
		*	Upon destruction, Alternative will delete each individual guard
		*	itself so you should not delete the guards yourself.
		*
		*	@param guardArray Pointer to the array of guard pointers
		*	@param amount The length of guardArray
		*/
		Alternative(Guard** const guardArray,const unsigned int amount);

		/**
		*	@overload
		*/
		inline Alternative(const std::list<Guard*>& _guards)
			:	guards(_guards.begin(),_guards.end()),favourite(0)
		{
		}
		
		/**
		*	@overload
		*/
		inline Alternative(const std::vector<Guard*>& _guards)
			:	guards(_guards),favourite(0)
		{
		}

		/**
		*	@overload
		*
		*	ITERATOR must be an iterator similar to std::vector<Guard*>::const_iterator or std::list<Guard*>::const_iterator.  It 
		*	will be passed to the constructor of std::vector<Guard*>.  It is not advisable that you use std::set<Guard*>::iterator,
		*	because the order of the guards is important, and std::set will order the guards somewhat arbitrarily.
		*/
		template <typename ITERATOR>
		inline Alternative(ITERATOR begin,ITERATOR end)
			:	guards(begin,end),favourite(0)
		{
		}
		
		/**
		*	Destructor.
		*
		*	It destroys all the guards in its guard array
		*/
		~Alternative();

		/**
		*	Performs a <tt>PRI ALT</tt> on the guard array
		*
		*	A <tt>PRI ALT</tt> returns when one of the guards is ready.
		*	If more than one is ready then the earliest in the guard array
		*	is always picked.
		*
		*	@return The index of the guard that was picked
		*/
		unsigned int priSelect();

		/**
		*	Performs a fair <tt>ALT</tt> on the guard array
		*
		*	A fair <tt>ALT</tt> returns when one of the guards is active.
		*	Over a number of calls it evens out the precedence
		*	given to each of the guards so that they are all
		*	given equal chance (over many calls) of being selected over the other guards.
		*
		*	It is equivalent to performing a <tt>PRI ALT</tt> while
		*	cycling the order of the guards each call.
		*
		*	@return The index of the guard that was picked
		*/
		unsigned int fairSelect();
		
		/**
        *   Performs a "same" <tt>ALT</tt> on the guard array
        *
        *   A "same" <tt>ALT</tt> returns when one of the guards is active.
        *	Each time it gives precedence to the guard that was activated last time.
        *
        *	It is useful for multiplexors where continual data bursts are likely
        *	to come on a channel, to speed up inputting from the same channel many
        *	times in a row.
        *
        *   @return The index of the guard that was picked
        */
        unsigned int sameSelect();

		/**
		*	Replaces a guard in the array.
		*
		*	This method is useful for changing timeout guards
		*	in an ALT without constructing a new ALT.
		*
		*	@param index The index of the guard to replace
		*	@param guard The new guard
		*	@return The old guard, must be deleted or otherwise dealt with
		*/
		Guard* replaceGuard(unsigned int index,Guard* guard);

	}; //class Alternative

} //namespace csp
