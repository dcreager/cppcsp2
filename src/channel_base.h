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

/** @file channel_base.h
*	@internal
*	@brief Contains the header for the internal channel-related classes, look at channel.h
*
*	This file is \#included from channel.h
*/

#ifndef INCLUDED_FROM_CPPCSP_H
#error This file should only be included by csp.h, not individually
#endif

namespace csp
{
	class ParCommItem;
	
	template <typename T>
	class Chanin;
	template <typename T>
	class AltChanin;
	template <typename T>
	class Chanout;		
	
	template <typename DATA_TYPE>
	class ScopedExtInput;

namespace internal
{

	/**@internal
	*	The highest parent of all the channel classes
	*
	*	As detailed in the report, this class is provided so that both
	*	BaseChan and BaseNormalChan/BaseBufferedChan can share the same
	*	poison information and hence it is virtually inherited by both
	*
	*	Note that a pointer to this class should never be held and hence it
	*	contains no virtual destructor
	*
	*	@see BaseChan
	*	@see BaseNormalChan
	*	@see BaseBufferedChan	
	*/
	class PoisonableChan : public boost::noncopyable
	{
	protected:
		/**@internal
		*	The poisoned flag for the channel (true means it is is poisoned)
		*/
		volatile bool isPoisoned;

	public:
		inline PoisonableChan()
			:	isPoisoned(false)
		{
		}
	};

	/**@internal
	*	The templated class containing all the virtual function definitions for all the channel classes.
	*
	*	@see PoisonableChan
	*	@see BaseAltChan
	*/
	template <typename DATA_TYPE>
	class BaseChan : protected virtual PoisonableChan
	{
	protected:				
	
		inline virtual ~BaseChan()
		{
		}
	
		/**@internal
		*	Performs a normal input on this channel
		*
		*	@param dest The destination for the input
		*/
		virtual void input(DATA_TYPE* const dest) = 0;

		/**@internal
		*	Starts an extended input on this channel.
		*
		*	It will only return when the extended part of the input is to
		*	be performed, i.e. after the input but before freeing the outputter
		*
		*	@param dest The destination for the input
		*/
		virtual void beginExtInput(DATA_TYPE* const dest) = 0;

		/**@internal
		*	Ends an extended input on this channel
		*
		*	It should be called after the extended part of the input has
		*	been performed so that the outputter can be freed.
		*
		*	This method should never throw a PoisonException.
		*	If the channel was poisoned by the writer, it would have been noticed
		*	by beginExtInput.  If the reader has poisoned it since, they should
		*	not get the exception thrown back at them when ending the extended input - instead,
		*	nothing will happen
		*/
		virtual void endExtInput()  = 0;

		/**@internal
		*	Performs a normal input on this channel
		*
		*	@param source The source for the output
		*/
		virtual void output(const DATA_TYPE* const source) = 0;
	
		/**@internal
		*	Poisons the channel.
		*
		*	This is a virtual function because poisoning the channel does not
		*	just involve setting the flag (which classes overriding this must
		*	do), it also involves waking up anyone waiting on the channel,
		*	which is implementation dependent.
		*
		*	This function should be capable of being called multiple times
		*	though it can just check the flag and return if the channel is already
		*	poisoned
		*/
		virtual void poisonIn() = 0;
		
		/**@internal
		*	Poisons the channel.
		*
		*	This is a virtual function because poisoning the channel does not
		*	just involve setting the flag (which classes overriding this must
		*	do), it also involves waking up anyone waiting on the channel,
		*	which is implementation dependent.
		*
		*	This function should be capable of being called multiple times
		*	though it can just check the flag and return if the channel is already
		*	poisoned
		*/
		virtual void poisonOut() = 0;

	public:

		/**@internal
		*	Chanin is a friend because:
		*
		*	It needs to access these functions for the channels
		*/		
		friend class Chanin<DATA_TYPE>;
		
		/**@internal
		*	AltChanin is a friend because:
		*
		*	It needs to access these functions for the channels
		*/		
		friend class AltChanin<DATA_TYPE>;		

		/**@internal
		*	Chanout is a friend because:
		*
		*	It needs to access these functions for the channels
		*/
		friend class Chanout<DATA_TYPE>;
		
		friend class ScopedExtInput<DATA_TYPE>;
		
		typedef BaseChan<DATA_TYPE>* Pointer;

	}; //BaseChan<DATA_TYPE>

	template <typename DATA_TYPE>
	class BaseAltChan : public BaseChan<DATA_TYPE>
	{
	protected:
		/**@internal
		*	Gets a normal input guard for this class
		*
		*	@param dest The destination for the input
		*	@return A pointer to the guard
		*	@see Guard
		*	@see Alternative
		*/
		virtual Guard* inputGuard() = 0;

		/**@internal
		*	Checks whether an input would succeed immediately on the channel
		*
		*	This is used as a short form of polling a channel
		*	(that would otherwise have to be done using a <tt>PRI ALT</tt>)
		*
		*	@return Whether an input would succeed immediately on this channel
		*/
		virtual bool pending() = 0;
		
	public:	
		/**@internal
		*	AltChanin is a friend because:
		*
		*	It needs to access these functions for the channels
		*/		
		friend class AltChanin<DATA_TYPE>;	

		typedef BaseAltChan<DATA_TYPE>* Pointer;

	};


} //namespace internal
} //namespace csp
