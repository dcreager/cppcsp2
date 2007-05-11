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

/** @file channel_ends.h
*	@brief Contains the header for the channel-end classes (Chanin, AltChanin and Chanout)
*/

#ifndef INCLUDED_FROM_CPPCSP_H
#error This file should only be included by csp.h, not individually
#endif

namespace csp
{
	template <typename DATA_TYPE>
	class ScopedExtInput;
	
	template <typename DATA_TYPE>
	Chanin<DATA_TYPE> NoPoison(const Chanin<DATA_TYPE>& in);
	
	template <typename DATA_TYPE>
	AltChanin<DATA_TYPE> NoPoison(const AltChanin<DATA_TYPE>& in);
	
	template <typename DATA_TYPE>
	Chanout<DATA_TYPE> NoPoison(const Chanout<DATA_TYPE>& out);
		
	template<typename DATA_TYPE>
	class Chanin
	{
	private:
		/**@internal
		*	The channel for which this is a reading end
		*/
		typename internal::BaseChan<DATA_TYPE>::Pointer channel;	
		
		/**@internal
		*	Is it poisonable?
		*/
		bool canPoison;
	
	public:
		/**@internal
		*	A constructor for internal use only
		*		
		*	While it is ugly to have this constructor public, it is not
		*	unsafe as all the channels inherit from BaseChan using protected
		*	so the user cannot get a pointer to pass, except by an illegal cast
		*	which we cannot prevent anyway.
		*
		*	The alternative would have been to have eight friend declarations
		*	(one for each channel type), this way also allows any new channel
		*	types that implement BaseChan to be able to use Chanin and Chanout
		*	straight away (i.e. no need to add a friend declaration anywhere).
		*/
		inline Chanin(internal::BaseChan<DATA_TYPE>* const ch,bool _canPoison)
			:	channel(ch),canPoison(_canPoison)
		{
		}
	
		/**
		*	A standard copy constructor
		*/
		inline Chanin(const Chanin<DATA_TYPE>& cho)
			:	channel(cho.channel),canPoison(cho.canPoison)
		{
		}
				

		/**
		*	A constructor for instances that are class members
		*
		*	Do not use this constructor for constructing channel ends,
		*	instead use something like this:
			@code
				Chanin<int> chanin = somechannel.reader();
			@endcode
		*	This constructor is provided because, as can be seen in the Delta
		*	common process, sometimes channel ends need to be class members but they
		*	cannot be initialised until the <i>body</i> of the constructor (rather than the
		*	initialiser list) so this default constructor is provided for that mode of use.
		*/
		inline Chanin()
			:	channel(NULL),canPoison(true)
		{
		}		

		/**
		*	Performs a normal input
		*
		*	This simply performs a normal input on the channel, into dest
		*	It will not return until the input has completed
		*
		*	@param dest The destination of the input.  Must not be NULL
		*/
		inline void input(DATA_TYPE* const dest) const
		{
			channel->input(dest);
		};

		/**
		*	Identical to the input method
		*
		*	@see input()
		*/
		inline void read(DATA_TYPE* const dest) const
		{
			channel->input(dest);
		};

		/**
		*	Performs a normal input
		*	
		*	Does the same as the input() function but takes a reference not
		*	a pointer.  It can be used to easily chain sequential inputs as follows:
			@code
				One2OneChannel<int> chan;
				Chanin<int> in = chan.reader();
				int a,b;

				//This:
				in.input(&a);in.input(&b);

				//Is the same as this:
				in >> a >> b;
			@endcode
		*
		*	@param obj The variable to input into
		*	@return This channel, for chaining inputs
		*/
		inline const Chanin<DATA_TYPE>& operator>> (DATA_TYPE& obj) const
		{
			input(&obj);
			return *this;
		}

		/**
		*	Poisons the channel
		*
		*	This poisons the channel, and can be safely called multiple times
		*
		*	Read about PoisonException for more information regarding poison
		*
		*	@see PoisonException
		*	@see checkPoison()
		*/
		inline void poison() const
		{
			if (canPoison)
				channel->poisonIn();
		};

		/**
		*	Checks the channel for poison
		*
		*	Rather than being an accessor for the poison flag, this function
		*	instead checks it and then returns normally if the channel is not
		*	poisoned, or throws a PoisonException if it is.
		*
		*	This function allows processes performing lots of calculations and
		*	no channel communications to check for poison more often than 
		*	only checking when a channel communication is performed
		*
		*	@see PoisonException
		*	@see poison()
		*/
		inline void checkPoison() const
		{
			if (channel->isPoisoned) throw PoisonException();
		}		
		
		/**
		*	To allow sets of channel ends that are able to keep only one end per channel we define relational operators
		*	Note that a non-poisonable channel end is not equal to a poisonable channel end of the same channel
		*/		
		inline bool operator==(const csp::Chanin<DATA_TYPE>& b) const;		

		inline bool operator<(const csp::Chanin<DATA_TYPE>& b) const
		{
			if (channel < b.channel)
				return true;
			else if (channel == b.channel)
				return ((canPoison ? 1 : 0) < (b.canPoison ? 1 : 0));
			else
				return false;
		};
		

		/**
		*	DEPRECATED; A syntax for performing an extended input.  
		*
		*	This function takes a destination for the extended input, a class type -
		*	which will usually be a process, <em>though it can be any class</em> - and
		*	a member function of that class (which should be accessible by Chanin).
		*
		*	The input will be performed, then the function will be called then the
		*	outputter will be freed.
		*
		*	You may well wish to look at ScopedExtInput for an alternative way of
		*	performing extended inputs.
		*
		*	@param dest The destination for the extended input.  Must not be NULL
		*	@param proc The class (not necessarily a process) to call memberFn for
		*	@param memberFn The member function of proc to call		
		*
		*	@see ScopedExtInput
		*	@deprecated
		*/
		template <class PROCESS>
		void extInput(DATA_TYPE* const dest,PROCESS* proc,void (PROCESS::*memberFn)()) const
		{
			channel->beginExtInput(dest);
			(proc->*memberFn)();
			channel->endExtInput();
		}

		/**
		*	DEPRECATED; Performs an extended input.  
		*
		*	This function inputs to dest, then calls function() then frees the outputter.
		*	You may well wish to look at ScopedExtInput for an alternative way of
		*	performing extended inputs
		*
		*	@param dest The destination for the extended input.  Must not be NULL
		*	@param function The function to call during the extended input
		*
		*	@see ScopedExtInput
		*	@deprecated
		*/

		void extInput(DATA_TYPE* const dest,void (*function)()) const
		{
			channel->beginExtInput(dest);
			function();
			channel->endExtInput();
		}
		
		friend class ScopedExtInput<DATA_TYPE>;
		
		/**
		*	A typedef for the ScopedExtInput.  Chanin<DATA_TYPE>::ScopedExtInput
		*	is equivalent to ScopedExtInput<DATA_TYPE>
		*
		*/
		typedef ScopedExtInput<DATA_TYPE> ScopedExtInput;
		
		friend Chanin<DATA_TYPE> NoPoison<>(const Chanin<DATA_TYPE>& in);
		
	}; //class Chanin<DATA_TYPE>


	template<typename DATA_TYPE>
	class AltChanin
	{
	private:
		/**@internal
		*	The channel for which this is a reading end
		*/
		typename internal::BaseAltChan<DATA_TYPE>::Pointer channel;		
		
		/**@internal
		*	Is it poisonable?
		*/
		bool canPoison;		
	public:
		/**@internal
		*	A constructor for internal use only
		*		
		*	While it is ugly to have this constructor public, it is not
		*	unsafe as all the channels inherit from BaseChan using protected
		*	so the user cannot get a pointer to pass, except by an illegal cast
		*	which we cannot prevent anyway.
		*
		*	The alternative would have been to have eight friend declarations
		*	(one for each channel type), this way also allows any new channel
		*	types that implement BaseChan to be able to use Chanin and Chanout
		*	straight away (i.e. no need to add a friend declaration anywhere).
		*/
		inline AltChanin(internal::BaseAltChan<DATA_TYPE>* const ch,bool _canPoison)
			:	channel(ch),canPoison(_canPoison)
		{
		}
	
		/**
		*	A standard copy constructor
		*/
		inline AltChanin(const AltChanin<DATA_TYPE>& cho)
			:	channel(cho.channel),canPoison(cho.canPoison)
		{
		}

		/**
		*	A constructor for instances that are class members
		*
		*	Do not use this constructor for constructing channel ends,
		*	instead use something like this:
			@code
				Chanin<int> chanin = somechannel.reader();
			@endcode
		*	This constructor is provided because, as can be seen in the Delta
		*	common process, sometimes channel ends need to be class members but they
		*	cannot be initialised until the <i>body</i> of the constructor (rather than the
		*	initialiser list) so this default constructor is provided for that mode of use.
		*/
		inline AltChanin()
			:	channel(NULL), canPoison(true)
		{
		}
		
		/**
		*	Returns a copy with the Alt capability hidden.  This is useful for
		*	passing the channel end to processes that do not need the ALTing capability
		*/
		operator Chanin<DATA_TYPE> () const
		{
			return Chanin<DATA_TYPE>(channel,canPoison);
		};

		/**
		*	Gets an input guard to use in an <tt>ALT</tt>.
		*
		*	The guard will be ready when either there is data to be read, or the channel is poisoned.  If an input guard
		*	for a channel is selected by the Alternative, you must then perform an input or extended input
		*	on the channel as your next action.
		*
		*	The Alternative class will delete this Guard, so you don't have to
		*
		*	@section compat C++CSP v1.x Compatibility
		*
		*	This method has been changed since C++CSP v1.x.  In v1.x, the inputGuard method took a parameter that specified
		*	where to store the result of the input; the Alternative would then perform the input automatically if that
		*	guard was selected, so after the select method returned there was no need to perform the input.
		*
		*	The JCSP method has now been adopted.  If the guard is selected by the Alternative, it is now up to the user
		*	to then perform the input from the channel manually.  However, this gives the added flexibility of having a 
		*	custom try-catch block for poison, or the use of the new ScopedExtInput class for an in-place extended input.
		*
		*	@see Alternative
		*		
		*	@return The input guard
		*/
		inline Guard* inputGuard() const
		{
			return channel->inputGuard();
		};
		
		/**
		*	Performs a normal input
		*	
		*	Does the same as the input() function but takes a reference not
		*	a pointer.  It can be used to easily chain sequential inputs as follows:
		*	@code
				One2OneChannel<int> chan;
				AltChanin<int> in = chan.reader();
				int a,b;

				//This:
				in.input(&a);in.input(&b);

				//Is the same as this:
				in >> a >> b;
			@endcode
		*
		*	@param obj The variable to input into
		*	@return This channel, for chaining inputs
		*/
		inline const AltChanin<DATA_TYPE>& operator>> (DATA_TYPE& obj) const
		{
			input(&obj);
			return *this;
		};
		
		/**
		*	Checks whether an input would finish immediately on the channel
		*
		*	This is used as a short form of polling a channel
		*	(that would otherwise have to be done using a <tt>PRI ALT</tt> with a skip guard)
		*
		*	In practical terms, this method will return true if any of the following are true:
		*	- On an unbuffered channel, a writer is waiting to write data to the channel 
		*	- On a buffered channel, the buffer is not empty
		*	- The channel is poisoned		
		*		
		*	@return Whether an input would finish immediately on this channel
		*/
		inline bool pending() const
		{
			return channel->pending();
		};		

		/**
		*	Performs a normal input
		*
		*	This simply performs a normal input on the channel, into dest
		*	It will not return until the input has completed
		*
		*	@param dest The destination of the input.  Must not be NULL
		*/
		inline void input(DATA_TYPE* const dest) const
		{
			channel->input(dest);
		};

		/**
		*	Identical to the input method
		*
		*	@see input()
		*/
		inline void read(DATA_TYPE* const dest) const
		{
			channel->input(dest);
		};


		/**
		*	Poisons the channel
		*
		*	This poisons the channel, and can be safely called multiple times
		*
		*	Read about PoisonException for more information regarding poison
		*
		*	@see PoisonException
		*	@see checkPoison()
		*/
		inline void poison() const
		{
			if (canPoison)
				channel->poisonIn();
		}

		/**
		*	Checks the channel for poison
		*
		*	Rather than being an accessor for the poison flag, this function
		*	instead checks it and then returns normally if the channel is not
		*	poisoned, or throws a PoisonException if it is.
		*
		*	This function allows processes performing lots of calculations and
		*	no channel communications to check for poison more often than 
		*	only checking when a channel communication is performed
		*
		*	@see PoisonException
		*	@see poison()
		*/
		inline void checkPoison() const
		{
			if (channel->isPoisoned) throw PoisonException();
		}		
		
		/**
		*	To allow sets of channel ends that are able to keep only one end per channel we define relational operators
		*	Note that a non-poisonable channel end is not equal to a poisonable channel end of the same channel
		*/
		
		inline bool operator==(const csp::AltChanin<DATA_TYPE>& b) const;		

		inline bool operator<(const csp::AltChanin<DATA_TYPE>& b) const
		{
			if (channel < b.channel)
				return true;
			else if (channel == b.channel)
				return ((canPoison ? 1 : 0) < (b.canPoison ? 1 : 0));
			else
				return false;
		};
		
		/**
		*	DEPRECATED; A syntax for performing an extended input.  
		*
		*	This function takes a destination for the extended input, a class type -
		*	which will usually be a process, <em>though it can be any class</em> - and
		*	a member function of that class (which should be accessible by Chanin).
		*
		*	The input will be performed, then the function will be called then the
		*	outputter will be freed.
		*
		*	You may well wish to look at ScopedExtInput for an alternative way of
		*	performing extended inputs.
		*
		*	@param dest The destination for the extended input.  Must not be NULL
		*	@param proc The class (not necessarily a process) to call memberFn for
		*	@param memberFn The member function of proc to call		
		*
		*	@see ScopedExtInput
		*	@deprecated
		*/
		template <class PROCESS>
		void extInput(DATA_TYPE* const dest,PROCESS* proc,void (PROCESS::*memberFn)()) const
		{
			channel->beginExtInput(dest);
			(proc->*memberFn)();
			channel->endExtInput();
		};

		/**
		*	DEPRECATED; Performs an extended input.  
		*
		*	This function inputs to dest, then calls function() then frees the outputter.
		*	You may well wish to look at ScopedExtInput for an alternative way of
		*	performing extended inputs
		*
		*	@param dest The destination for the extended input.  Must not be NULL
		*	@param function The function to call during the extended input
		*
		*	@see ScopedExtInput
		*	@deprecated
		*/

		void extInput(DATA_TYPE* const dest,void (*function)()) const
		{
			channel->beginExtInput(dest);
			function();
			channel->endExtInput();
		}
		
		/**
		*	A typedef for the ScopedExtInput.  Chanin<DATA_TYPE>::ScopedExtInput
		*	is equivalent to ScopedExtInput<DATA_TYPE>
		*
		*/
		typedef ScopedExtInput<DATA_TYPE> ScopedExtInput;
		
		
		friend AltChanin<DATA_TYPE> NoPoison<>(const AltChanin<DATA_TYPE>& in);
		

	}; //class AltChanin<DATA_TYPE>


	template <typename DATA_TYPE>
	class ScopedExtInput
	{
	private:
		/**@internal
		*	The channel end we are performing the extended input on
		*/
		const Chanin<DATA_TYPE> in;
	public:
		/**
		*	Constructs a ScopedExtInput, beginning an extended input on the designated channel.
		*
		*	See the documentation at the top of this page for details on how to use this class.
		*	
		*	@param _in The channel input end to perform the extended input on
		*	@param dest The destination for the extended input.  Must not be NULL
		*/
		inline ScopedExtInput(const Chanin<DATA_TYPE>& _in,DATA_TYPE* const dest)
			:	in(_in)
		{
			in.channel->beginExtInput(dest);
		}	
		
		/**
		*	Destroys the ScopedExtInput object.
		*
		*	This will finish the extended input and free the writer
		*/
		inline ~ScopedExtInput()
		{
			in.channel->endExtInput();
		}
	};
	
	template<typename DATA_TYPE>
	class Chanout
	{
	private:
		/**@internal
		*	The channel for which this is a writing end
		*/
		typename internal::BaseChan<DATA_TYPE>::Pointer channel;

		/**@internal
		*	Is it poisonable?
		*/
		bool canPoison;

	public:
		/**@internal
		*	A constructor for internal use only
		*		
		*	While it is ugly to have this constructor public, it is not
		*	unsafe as all the channels inherit from BaseChan using protected
		*	so the user cannot get a pointer to pass, except by an illegal cast
		*	which we cannot prevent anyway.
		*
		*	The alternative would have been to have eight friend declarations
		*	(one for each channel type), this way also allows any new channel
		*	types that implement BaseChan to be able to use Chanin and Chanout
		*	straight away (i.e. no need to add a friend declaration anywhere).
		*/
		explicit Chanout(internal::BaseChan<DATA_TYPE>* const ch,bool _canPoison)
			:	channel(ch),canPoison(_canPoison)
		{
		}

		/**
		*	A standard copy constructor
		*/
		Chanout(const Chanout<DATA_TYPE>& cho)
			:	channel(cho.channel),canPoison(cho.canPoison)
		{
		}

		/**
		*	A constructor for instances that are class members
		*
		*	Do not use this constructor normally for constructing channel ends,
		*	instead use something like this:
			@code
				Chanout<int> chanout = somechannel.writer();
			@endcode
		*	This constructor is provided because, as can be seen in the Delta
		*	common process, sometimes channel ends need to be class members but they
		*	cannot be initialised until the <i>body</i> of the constructor (rather than the
		*	initialiser list) so this default constructor is provided for that mode of use.
		*/
		Chanout()
			:	channel(NULL)
		{
		}

		/**
		*	Performs a normal output
		*
		*	This simply performs a normal output on the channel, from source
		*	It will not return until the output has completed
		*
		*	@param source The source of the output.  Must not be NULL
		*/
		void output(const DATA_TYPE* source) const
		{
			channel->output(source);
		};

		/**
		*	Identical to the output method
		*
		*	@see output()
		*/
		void write(const DATA_TYPE* source) const
		{
			channel->output(source);
		};

		/**
		*	Performs a normal output
		*	
		*	Does the same as the output() function but takes a reference not
		*	a pointer.  It can be used to easily chain sequential outputs as follows:
			@code
				One2OneChannel<int> chan;
				Chanout<int> out = chan.writer();
				int a,b;

				//This:
				in.output(&a);in.output(&b);

				//Is the same as this:
				in << a << b;
			@endcode
		*
		*	@param obj The variable to output from
		*	@return This channel, for chaining outputs
		*/
		inline const Chanout<DATA_TYPE>& operator<< (const DATA_TYPE& obj) const
		{
			output(&obj);
			return *this;
		};

		/**
		*	Poisons the channel
		*
		*	This poisons the channel, and can be safely called multiple times
		*
		*	Read about PoisonException for more information regarding poison
		*
		*	@see PoisonException
		*	@see checkPoison()
		*/
		inline void poison() const {if (canPoison) channel->poisonOut();};

		/**
		*	Checks the channel for poison
		*
		*	Rather than being an accessor for the poison flag, this function
		*	instead checks it and then returns normally if the channel is not
		*	poisoned, or throws a PoisonException if it is.
		*
		*	This function allows processes performing lots of calculations and
		*	no channel communications to check for poison more often than 
		*	only checking when a channel communication is performed
		*
		*	@see PoisonException
		*	@see poison()
		*/
		inline void checkPoison() const
		{
			if (channel->isPoisoned) throw PoisonException();
		};

		/**
		*	To allow sets of channel ends that are able to keep only one end per channel we define relational operators
		*	Note that a non-poisonable channel end is not equal to a poisonable channel end of the same channel
		*/
		
		inline bool operator==(const csp::Chanout<DATA_TYPE>& b) const
		{
			return (channel == b.channel) && (canPoison == b.canPoison);
		};
		
		inline bool operator<(const csp::Chanout<DATA_TYPE>& b) const
		{
			if (channel < b.channel)
				return true;
			else if (channel == b.channel)
				return ((canPoison ? 1 : 0) < (b.canPoison ? 1 : 0));
			else
				return false;
		};		
		
		friend Chanout<DATA_TYPE> NoPoison<>(const Chanout<DATA_TYPE>& in);

	}; //class Chanout<DATA_TYPE>			

	/**
	*	@defgroup channelends Channel Ends
	*
	*	More information on using channels is available in the @ref channels "Channels" module, and
	*	in the @ref tut-channels "Channels" section of the guide.
	*
	*	Channels in C++CSP2 are accessed through their ends.  There are three channel-end types:
	*	- Chanout - a writing-end of a channel that allows normal writing of data
	*	- Chanin - a reading-end of a channel that allows normal reading of data, and extended inputs
	*	- AltChanin - a reading-end of a channel that allows normal reading of data, extended inputs, and ALTing (as well as extended ALTing)
	*
	*	These names may seem reversed if you think of them in a channel-oriented way (because Chanin is
	*	where you take data <i>out</i> of the channel).  However, instead think of them as process-oriented;
	*	the Chanin is where your process reads data from the channel.
	*
	*	The channel ends can be obtained through the reader() and writer() methods of the channels, such
	*	as One2OneChannel and BufferedOne2OneChannel.  The designation of the channels details whether
	*	channels can be shared.  For example, a One2OneChannel is for use by one writer and one reader.
	*	A One2AnyChannel is for one writer and any number of readers.  You can read more about this
	*	on the page about @ref channels.  It is up to you, the programmer, to make sure that these
	*	channel-ends are not used inappropriately.  If you share a channel-end that should not be shared,
	*	it is likely to crash your program or cause side-effects.
	*
	*	You will notice that there are two types of reading channel ends; Chanin and AltChanin.  The reason
	*	for this divide is because not all channels support ALTing - specifically, the shared reading-end
	*	channels (One2AnyChannel, Any2AnyChannel, BufferedOne2AnyChannel and BufferedAny2OneChannel) do
	*	not support ALTing.  Therefore their reader() function provides a Chanin, whereas all the other
	*	classes return an AltChanin from their reader() method.
	*
	*	You will need to decide, when programming a new process, whether it should use a Chanin or an
	*	an AltChanin.  The simple solution is that you should use an AltChanin if you need to ALT over
	*	the channel, but otherwise use a Chanin.  That is, use the minimum that you require.
	*
	*	An AltChanin can be implicitly converted into a Chanin, so there is no problem in passing an
	*	AltChanin to a process that requires a Chanin.  However, the reverse transformation is not possible,
	*	so a process that needs an AltChanin cannot be given a Chanin.	
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	C++CSP v1.x had only two channel-ends: Chanout and Chanin.  Chanin contained methods for ALTing
	*	but threw the ugly FunctionNotSupportedException if the underlying channel did not allow ALTing.
	*	This was not a good solution, and so Chanin has now been divided into Chanin and AltChanin.
	*	
	*	The other change is that the ParallelComm system has been removed.  The implementation that allowed
	*	this quick optimisation has now changed, and so it became untenable to keep it.  Besides which,
	*	it was always just a quick way of doing something that could be accomplished in a more generic manner.
	*	
	*	For example, where you could previously have used a ParallelComm to communicate in parallel on two
	*	output channels (say, c and d), you should now do it as follows:
	*	@code
		Run(InParallel
			(new WriterProcess(c.writer(),6))
			(new WriterProcess(d.writer(),42))
		);
		@endcode
	*
	*	This now closely mirrors the JCSP way of accomplishing the same thing.
	*	
	*	@{
	*/
	
	/** @class Chanout
	*
	*	The writing end of a channel.
	*
	*	This class is returned by functions like One2OneChannel::writer()
	*	to return a writing end of a channel (that carries items of type DATA_TYPE).
	*	More information can be found on the @ref channelends "Channel Ends" page and
	*	in the @ref tut-channels "Channels" section of the guide.
	*
	*	They are small classes (the size of a pointer and a boolean) that can be assigned
	*	around but <em>only one writing end on a channel should be in use
	*	at any one time for the one-to-one and one-to-any channel types</em>.
	*
	*	The mobile channel-ends paradigm may be implemented by simply encasing
	*	the channel end in a mobile, i.e. Mobile< Chanout<int> > for ints
	*
	*	This item can be used for normal outputting in one of two ways, either by using
	*	the output()/write() methods or by using the \<\< operator so for example:
	*	@code
			One2OneChannel<int> chan;
			Chanout<int> out = chan.writer();
			int a,b;

			//This:
			out.output(&a);out.write(&b);

			//Is the same as this:
			out << a << b;
		@endcode
	*
	*	Note that the output/write functions take a pointer (rather than references) 
	*	to make the fact that the location is assigned from, whereas for simplicity the \<\< operator
	*	uses references, as out \<\< a is more natural than out \<\< \&a.
	*
	*	This class can also be declared const easily as all its methods are const.
	*
	*	You should also read about the PoisonException that could be thrown,
	*	to understand poisoning channels.
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	See the note in the @ref channelends "Channel Ends" module
	*
	*	@see One2OneChannel::reader()
	*	@see One2AnyChannel::reader()
	*	@see Any2OneChannel::reader()
	*	@see Any2AnyChannel::reader()
	*	@see BufferedOne2OneChannel::reader()
	*	@see BufferedOne2AnyChannel::reader()
	*	@see BufferedAny2OneChannel::reader()
	*	@see BufferedAny2AnyChannel::reader()
	*	@see AltChanin	
	*	@see Chanin
	*	@see PoisonException
	*/

	
	/** @class Chanin
	*
	*	The reading end of a channel.
	*
	*	This class is returned by functions like One2AnyChannel::reader()
	*	to return a reading end of a channel (that carries items of type DATA_TYPE).
	*	More information can be found on the @ref channelends "Channel Ends" page and
	*	in the @ref tut-channels "Channels" section of the guide.
	*
	*	They are small classes (the size of a pointer and a boolean) that can be assigned
	*	around but <em>only one reading end on a channel should be in use
	*	at any one time for the one-to-one and any-to-one channel types</em>.
	*
	*	The mobile channel-ends paradigm may be implemented by simply encasing
	*	the channel end in a mobile, i.e. Mobile< Chanin<int> > for ints
	*
	*	This item can be used for normal inputting in one of two ways, either by using
	*	the input() or read() methods or by using the \>\> operator so for example:
		@code
			One2OneChannel<int> chan;
			Chanin<int> in = chan.reader();
			int a,b;

			//This:
			in.input(&a);in.read(&b);

			//Is the same as this:
			in >> a >> b;
		@endcode
	*
	*	Note that the input/read functions take a pointer (rather than references)
	*	to emphasise that the location is assigned to whereas for simplicity the \>\> operator
	*	uses references, as in \>\> a is more natural than in \>\> \&a.
	*
	*	This class can be declared const easily as all of its methods are const.
	*
	*	Chanin does not support ALTing (see the Alternative class for details).  This is
	*	because the channels with shared reading ends, such as One2AnyChannel, do not
	*	allow ALTing.  Channels that do allow ALTing, such as One2OneChannel, provide
	*	an AltChanin that does allow ALTing.
	*
	*	You should also read about the PoisonException that could be thrown,
	*	to understand poisoning channels.
	*
	*	<b>C++CSP v1.x Compatibility:</b>
	*
	*	See the note in the @ref channelends "Channel Ends" module
	*	
	*	@see One2AnyChannel::reader()	
	*	@see Any2AnyChannel::reader()	
	*	@see BufferedOne2AnyChannel::reader()	
	*	@see BufferedAny2AnyChannel::reader()
	*	@see AltChanin	
	*	@see Chanout
	*	@see PoisonException
	*/
	
	/** @class AltChanin
	*
	*	This class is identical to Chanin except that it also supports ALTing.  See the page on @ref channelends "Channel Ends" and
	*	the section on @ref tut-channels "Channels" in the guide.
	*
	*	<b>C++CSP v1.x Compatibility:</b>
	*
	*	See the note in the @ref channelends "Channel Ends" module
	*
	*	@see One2OneChannel::reader()
	*	@see Any2OneChannel::reader()
	*	@see BufferedOne2OneChannel::reader()
	*	@see BufferedAny2OneChannel::reader()
	*	@see Chanin
	*	@see Chanout
	*	@see PoisonException
	*/
	
	/** @class ScopedExtInput
	*
	*	Provides an easy way to perform extended inputs using scope.
	*
	*	An extended input needs a beginning and an end, with the extended action inbetween.  The Chanin::extInput (and AltChanin::extInput) functions
	*	provide one way to accomplish this, accepting a function that represents the extended action.
	*	However, this can be annoying where you want the extended action to use variables 
	*	declared in the function with the extended input.
	*
	*	Hence the ScopedExtInput class.  It is used as follows:
	*	@code
			One2OneChannel<int> c;
			int n;
			...
			{
				ScopedExtInput<int> ext(c.reader(),&n); //Extended input begins here
				
				//Extended action takes place in this block
				
			} //Extended input finishes here
		@endcode
	*
	*	It is advised that you begin a new block solely for the ScopedExtInput.  It aids readability
	*	and makes it clear where the extended input begins and ends.  Other uses may make the action less clear.
	*
	*	Some may feel that this class is dangerous, because the end of the extended input is hidden inside
	*	the end of the block.  There are other mechanisms available to perform an extended input, so
	*	you may use them instead.  The advantage of this method is that it allows a simple use of extended
	*	inputs, and prevents you forgetting to end the extended input.
	*	
	*	If an exception is thrown (be it a PoisonException or any other exception) that causes ScopedExtInput
	*	to be destroyed, the extended input is still finished correctly.	
	*
	*	More information can be found on the @ref scoped "Scoped Classes" page and in the 
	*	@ref tut-extinput "Extended Input" section of the guide.
	*
	*	@ingroup scoped
	*/

	/**
	*	@name NoPoison
	*
	*	Returns a non-poisonable version of the given channel end.
	*
	*	This can be used to supply non-poisonable channel ends to processes, for example:
	*	@code
			One2AnyChannel<int> c;
			One2OneChannel<int> d;
			Run(new Id<int>(NoPoison(c.reader()),d.writer()));
		@endcode
	*
	*	If you call poison() on a non-poisonable channel-end, it does not poison the channel (i.e. it has no effect).
	*	
	*	@{
	*/
	template <typename DATA_TYPE>
	inline Chanin<DATA_TYPE> NoPoison(const Chanin<DATA_TYPE>& in)
	{
		return Chanin<DATA_TYPE>(in.channel,false);
	}
		
	template <typename DATA_TYPE>
	inline AltChanin<DATA_TYPE> NoPoison(const AltChanin<DATA_TYPE>& in)
	{
		return AltChanin<DATA_TYPE>(in.channel,false);
	}
	
	template <typename DATA_TYPE>
	inline Chanout<DATA_TYPE> NoPoison(const Chanout<DATA_TYPE>& out)
	{
		return Chanout<DATA_TYPE>(out.channel,false);
	}		

	/** @} */ //end of NoPoison group
	
	/** @} */ //end of channel ends module	

} //namespace csp


