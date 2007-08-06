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

class ChannelTest;
class AltChannelTest;
class BufferedChannelTest;

#include <boost/type_traits.hpp>

namespace csp
{
	namespace internal
	{
		template <typename CHANNEL,typename DATA_TYPE,typename MUTEX>
		class Any2OneAdapter;
		
		template <typename CHANNEL,typename DATA_TYPE,typename MUTEX>
		class One2AnyAdapter;
	
	template <typename DATA_TYPE, typename MUTEX = internal::PureSpinMutex>	
	class _One2OneChannel : protected internal::BaseAltChan<DATA_TYPE>, private internal::Primitive
	{
	private:
		//typedef internal::QueuedMutex Mutex;
		typedef MUTEX Mutex;
		

	Process* volatile waiting;
    union {
        const DATA_TYPE* volatile src;
        DATA_TYPE* volatile dest;
    };
    using internal::BaseChan<DATA_TYPE>::isPoisoned;
    bool volatile * volatile commFinished;
    Mutex mutex;

    void checkPoison() {
        if (isPoisoned) {
            mutex.release();
            throw PoisonException();
        }
    }

    void input(DATA_TYPE* const paramDest) {
        mutex.claim();
        checkPoison();        

        if (waiting != NULL) {
            //Writer waiting:
            *paramDest = *src;
            Process* wasWaiting = waiting;
            waiting = NULL; src = NULL;
            *commFinished = true;
            freeProcessNoAlt(wasWaiting);
            mutex.release();
        } else {
            //No-one waiting:
            dest = paramDest;
            waiting = currentProcess();
            volatile bool finished = false;
            commFinished = &finished;
            mutex.release();
            reschedule();
            //Now communication has finished (or we've been poisoned)
            if (false == finished)
                throw PoisonException();
        }
    }

    void beginExtInput(DATA_TYPE* const paramDest) {
        mutex.claim();
        checkPoison();

        if (waiting != NULL) {
            //Writer waiting:
            *paramDest = *src;
            mutex.release();
        } else {
            //No-one waiting:
            dest = NULL;
            waiting = currentProcess();
            volatile bool finished = false;
            commFinished = &finished;
            mutex.release();
            reschedule();
            //Now we are in the extended action (or we've been poisoned)
            if (false == finished)
                throw PoisonException();
            mutex.claim();
            *paramDest = *src;
            mutex.release();
        }
    }

    //This method will never throw a poison exception
    void endExtInput() {
        //The reader may have poisoned since beginExtInput
        //If that is the case, we do not want to throw a PoisonException to them
        //The writer can't have poisoned (they are blocked) so it must have been the reader:
        //No need to claim the mutex

        if (false == isPoisoned) {
            Process* wasWaiting = waiting;
            waiting = NULL; src = NULL;
            *commFinished = true;
            freeProcessNoAlt(wasWaiting);
        }
    }

    void output(const DATA_TYPE* const paramSrc) {
        mutex.claim();
        checkPoison();

        if (waiting != NULL) {
            //Reader waiting:
            if (dest != NULL) {
                //Normal reader:
                *dest = *paramSrc;
                Process* wasWaiting = waiting;
                waiting = NULL; dest = NULL;
                *commFinished = true;
                freeProcessNoAlt(wasWaiting);
                mutex.release();
            } else {
                //Alting/extended reader:
                Process* wasWaiting = waiting;
                waiting = currentProcess();
                src = paramSrc;
                *commFinished = true;
                volatile bool finished = false;
                commFinished = &finished;
                freeProcessMaybe(wasWaiting);
                mutex.release();
                reschedule();
                //Now communication has finished (or we've been poisoned)
                if (false == finished)
                    throw PoisonException();
            }
        } else {
            //No-one waiting:
            src = paramSrc;
            waiting = currentProcess();
            bool finished = false;
            commFinished = &finished;
            mutex.release();
            reschedule();
            //Now communication has finished (or we've been poisoned)
            if (false == finished)
                throw PoisonException();
        }
    }

    void _poison() {
        mutex.claim();
        isPoisoned = true;
        //Don't change commFinished
        
        Process* wasWaiting = waiting;
        waiting = NULL;

        if (wasWaiting != NULL) {
            //Might be alting, might not:
            freeProcessMaybe(wasWaiting);
        }

        mutex.release();

    }

    void poisonOut() {
        _poison();
    }
    
    void poisonIn() {
        _poison();
    }
    
    class __ChannelGuard : public Guard
    {
    private:
		volatile bool finished;
		_One2OneChannel<DATA_TYPE,MUTEX>* channel;
	protected:
    
    
    bool enable(Process* proc) {
        channel->mutex.claim();

        if (channel->isPoisoned) {
            channel->mutex.release();
            return true;
        } else if (channel->waiting != NULL) {
        	if (channel->waiting == proc)
        	{
        		//This channel is being used multiple times in the alt, and clearly if we are waiting to read,
        		//no-one is waiting to write:
    	        channel->mutex.release();
        		return false;
        	}
        	else
        	{        
	            //Someone is ready to write
    	        channel->mutex.release();
    	        return true;
    	    }
        } else {
            //Put ourselves in the channel:
            channel->waiting = proc;
            channel->dest = NULL;
            //Mainly to stop the pointer being invalid:
            channel->commFinished = &finished;
            channel->mutex.release();
            return false;
        }
    }

    bool disable(Process* proc) {
        channel->mutex.claim();

        if (channel->isPoisoned) {
            channel->mutex.release();
            return true;
        } else if (channel->waiting != NULL && channel->waiting != proc) {
            //Someone is in the channel and its not us:
            channel->mutex.release();
            return true;
        } else {
            //Only us in the channel, remove ourselves:
            channel->waiting = NullProcessPtr;
            channel->mutex.release();
            return false;
        }
    }
    
    public:
			inline __ChannelGuard(_One2OneChannel<DATA_TYPE,MUTEX>* _channel)
				:	channel(_channel)
			{
			}
			
			virtual ~__ChannelGuard()
			{
			}

};
		
		bool pending()
		{
			bool ret = false;					
				
			mutex.claim();
				if (isPoisoned)
				{
					ret = true;
				}
				else if (waiting != NULL && src != NULL)
				{
					ret = true;
				}
			mutex.release();
			return ret;				
		}
		
		Guard* inputGuard()
		{			
			return new __ChannelGuard(this);
		}



	
	public:
		inline _One2OneChannel()
			:	waiting(NULL)
		{
			//Do both, just to be sure:
			src = NULL;
			dest = NULL;
		}
	
		AltChanin<DATA_TYPE> reader()
		{
			return AltChanin<DATA_TYPE>(this,true);
		}
		
		Chanout<DATA_TYPE> writer()
		{
			return Chanout<DATA_TYPE>(this,true);
		}
					
		template <typename CHANNEL,typename _DATA_TYPE,typename _MUTEX>
		friend class internal::Any2OneAdapter;
		
		template <typename CHANNEL,typename _DATA_TYPE,typename _MUTEX>
		friend class internal::One2AnyAdapter;
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::AltChannelTest;
		friend class ::BufferedChannelTest;
	};

	} //namespace internal
	
	template <typename DATA_TYPE>
	class One2OneChannel : public internal::_One2OneChannel<DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
	public:
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer()
		#endif
	};	
		
	template <typename DATA_TYPE>
	class BlackHoleChannel : private internal::BaseChan<DATA_TYPE>
	{
	private:
		using internal::BaseChan<DATA_TYPE>::isPoisoned;

		virtual void input(DATA_TYPE* const )
		{
		}

		virtual void beginExtInput(DATA_TYPE* const )
		{
		}

		virtual void endExtInput()
		{
		}
		
		virtual void output(const DATA_TYPE* const )
		{
			checkPoison();
		}
	
		virtual void poisonIn()
		{
			//should never be called
		}
		
		virtual void poisonOut()
		{
			isPoisoned = true;
		}
		
		void checkPoison()
		{
			if (isPoisoned)
				throw PoisonException();
		}
	public:
		/**
		*	Gets a writing end of the channel
		*/
		Chanout<DATA_TYPE> writer()
		{
			return Chanout<DATA_TYPE>(this,true);
		}
	};
	
	template <typename DATA_TYPE>
	class WhiteHoleChannel : private internal::BaseAltChan<DATA_TYPE>
	{
	private:
		DATA_TYPE data;
		using internal::BaseChan<DATA_TYPE>::isPoisoned;
		
	
		virtual void input(DATA_TYPE* const dest)
		{
			checkPoison();
			*dest = data;
		}

		virtual void beginExtInput(DATA_TYPE* const dest)
		{
			checkPoison();
			*dest = data;
		}

		virtual void endExtInput()
		{
			checkPoison();
			//Nothing to actually be done
		}
		
		virtual void output(const DATA_TYPE* const )
		{			
		}
	
		virtual void poisonIn()
		{
			isPoisoned = true;
		}
		
		virtual void poisonOut()
		{
			//should never be called			
		}
		
		void checkPoison()
		{
			if (isPoisoned)
				throw PoisonException();
		}
	public:
		/**
		*	Constructs a channel that forever outputs the specified value
		*
		*	@param _data The value to always output on the channel
		*/
		inline explicit WhiteHoleChannel(const DATA_TYPE& _data)
			:	data(_data)
		{
		}
	
		/**
		*	Gets a reading end of the channel
		*/
		AltChanin<DATA_TYPE> reader()
		{
			return AltChanin<DATA_TYPE>(this,true);
		}
	};
	
	namespace internal
	{	
		
		template <typename CHANNEL,typename DATA_TYPE,typename MUTEX = internal::QueuedMutex>
		class Any2OneAdapter : protected virtual CHANNEL
		{
		private:
			MUTEX writerMutex;
				
			virtual void output(const DATA_TYPE* const _src)
			{
				typename MUTEX::End end(writerMutex.end());
				end.claim();
					try
					{
						CHANNEL::output(_src);
					}
					catch (PoisonException&)
					{
						end.release();
						throw;
					}					
				end.release();
			}
			
			virtual void poisonOut()
			{
				typename MUTEX::End end(writerMutex.end());
				end.claim();
					CHANNEL::poisonOut();
				end.release();
			}
		public:
			inline Any2OneAdapter()
				:	CHANNEL()
			{
			}
		
			AltChanin<DATA_TYPE> reader()
			{
				return CHANNEL::reader();
			}
		
			Chanout<DATA_TYPE> writer()
			{
				return CHANNEL::writer();
			}
			
			//For testing:
			friend class ::ChannelTest;
			friend class ::BufferedChannelTest;
		};
		
		template <typename CHANNEL,typename DATA_TYPE,typename MUTEX = internal::QueuedMutex>
		class One2AnyAdapter : protected virtual CHANNEL
		{
		private:
			MUTEX readerMutex;
			typename MUTEX::End extEnd;
			bool inExtInput;
				
			virtual void input(DATA_TYPE* const _dest)
			{
				typename MUTEX::End end(readerMutex.end());
				end.claim();
					try
					{
						CHANNEL::input(_dest);
					}
					catch (PoisonException&)
					{
						end.release();
						throw;
					}
				end.release();
			}
			
			void beginExtInput(DATA_TYPE* const _dest)
			{				
				inExtInput = true;
				extEnd.claim();
					try
					{
						CHANNEL::beginExtInput(_dest);
					}
					catch (PoisonException&)
					{
						extEnd.release();
						inExtInput = false;
						throw;
					}
			}
			
			void endExtInput()
			{
				if (inExtInput)
				{
					try
					{
						CHANNEL::endExtInput();
					}
					catch (PoisonException&)
					{
						extEnd.release();
						inExtInput = false;
						throw;
					}
					
					extEnd.release();
					inExtInput = false;
				}
			}
			
			virtual void poisonIn()
			{
				if (false == inExtInput)
				{
					typename MUTEX::End end(readerMutex.end());
					end.claim();
						CHANNEL::poisonIn();
					end.release();
				}
				else
				{
					CHANNEL::poisonIn();
				}
			}
			
			//We don't need to override inputGuard or pending because our Chanin will never call them (only AltChanin would)
		public:
			inline One2AnyAdapter()
				:	CHANNEL(),extEnd(readerMutex.end()),inExtInput(false)
			{
			}
		
			//Not an AltChanin, because they're not allowed on 2Any channels
			Chanin<DATA_TYPE> reader()
			{
				return CHANNEL::reader();
			}
		
			Chanout<DATA_TYPE> writer()
			{
				return CHANNEL::writer();
			}
			
			//For testing:
			friend class ::ChannelTest;
			friend class ::BufferedChannelTest;
		};
		
		template <typename CHANNEL,typename DATA_TYPE>
		class Any2AnyAdapter : protected Any2OneAdapter<CHANNEL,DATA_TYPE>, protected One2AnyAdapter<CHANNEL,DATA_TYPE>
		{
		public:
			Chanin<DATA_TYPE> reader()
			{
				return Chanin<DATA_TYPE>(this,true);
			}
		
			Chanout<DATA_TYPE> writer()
			{
				return Chanout<DATA_TYPE>(this,true);
			}			
			
			//For testing:
			friend class ::ChannelTest;
			friend class ::BufferedChannelTest;
		};
	}
	
	template <typename DATA_TYPE>
	class Any2OneChannel : public internal::Any2OneAdapter<One2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{		
		//For testing:
		friend class ::ChannelTest;
	
	public:
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
		
	};
	
	
	template <typename DATA_TYPE>
	class One2AnyChannel : public internal::One2AnyAdapter<One2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
		//For testing:
		friend class ::ChannelTest;
	public:
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
	};
	
	
	template <typename DATA_TYPE>
	class Any2AnyChannel : public internal::Any2AnyAdapter<One2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
		//For testing:
		friend class ::ChannelTest;
	public:
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
	};
	
	
	/**
	*	@defgroup channels Channels
	*
	*	A channel is a way for processes to communicate data between each other.  A channel is 
	*	a one-directional, typed communication method between processes.  Because CSP principles
	*	prohibit the typical "shared data area" between processes, channels are the only way for processes
	*	to share data.
	*
	*	The channel objects themselves in C++CSP2 are relatively simple.  The only direct action any
	*	channel allows is to get reading- and writing-ends for the channel.  The channel ends are
	*	documented on the @ref channelends "Channel Ends" page.
	*
	*	@section unbuf Unbuffered Channels
	*
	*	The choice of channel object determines the underlying semantics of the channel.  A one-to-one
	*	channel (One2OneChannel) is to be used by one writer, one reader, and is unbuffered and synchronised.  That is, when a writer
	*	writes to the channel, it will not complete the write until the reader has read the value from
	*	the channel (and obviously vice versa - the reader cannot complete the read until the writer has
	*	written the value to the channel). @ref tut-channels "Unbuffered Channels" are also covered in the guide.
	*
	*	An Any2OneChannel is also unbuffered and synchronised but is for use by one reader and <i>one or more</i>
	*	writers.  As with a One2OneChannel, the communication does not happen until both the writer and
	*	the reader attempt to communicate.  However, with an Any2OneChannel, if multiple writers attempt
	*	to write to the channel, they will queue up and take it in turns to write to the channel.  Each
	*	write will require the reader to perform a read.
	*
	*	A One2AnyChannel is again unbuffered and synchronised but is for use by <i>one or more</i> readers
	*	and one writer.  This is <b>not</b> a broadcast channel.  If a writer writes a piece of data
	*	to the channel, then only one reader will receieve that data.  It will <b>not</b> replicate the data 
	*	to all readers.  Analogous to the Any2OneChannel, multiple readers attempting to read from the channel
	*	will queue up and take turns to read from the channel, and each reader will need the writer to
	*	perform a separate write.
	*
	*	An Any2AnyChannel combines the idea of an Any2OneChannel with a One2AnyChannel - it allows one or more
	*	readers and one or more writers.  Each writer attempting to write will be "paired" with a single reader
	*	that is attempting to read, and a communication will be performed between the two of them.  Simultaneous
	*	communications may not occur - this is only really relevant to extended rendezvous (also known as extended inputs).  
	*	While an extended rendezvous is taking place on the channel, another communication will not occur, 
	*	even if there is a reader and a writer both ready.  In this circumstance, consider using multiple channels
	*	or not using extended inputs.
	*
	*	Fairness is guaranteed on shared channels (Any2.. and ..2Any channels).  The exact ordering of channels
	*	may vary according to threaded nondeterminism, but a FIFO ordering is used that means that no channel end
	*	can possibly face infinite starvation in trying to use the channel.  @ref tut-shared "Shared channels" are
	*	covered in the guide.
	*
	*	Some users may wonder why they should not use an Any2AnyChannel in all circumstances.  Indeed, the
	*	performance hit of using an Any2AnyChannel rather than a One2OneChannel is not high.  The two main 
	*	reasons for not using Any2AnyChannel in every instance are:
	*	-	Although it is small, there is still a performance hit for using an Any2AnyChannel.  If you
	*		are programmming an application with large-scale parallelism this may become significant.
	*	-	The type of channel helps demonstrate what its mode of use will be.  If you declare a One2OneChannel
	*		it is clear that the channel will have one reader and one writer.  If all your channels are Any2AnyChannel
	*		it will not be immediately clear who might be using the channel, which can make interpreting your
	*		process network difficult.
	*	
	*	@section buf Buffered Channels
	*	
	*	All the channels described so far are unbuffered.  C++CSP2 also contains buffered channels, with
	*	a variety of different buffering strategies available.  The buffered channels are named:
	*	- BufferedOne2OneChannel
	*	- BufferedAny2OneChannel
	*	- BufferedOne2AnyChannel
	*	- BufferedAny2AnyChannel
	*
	*	Their semantics differ from the unbuffered channels solely according to the buffer that is used
	*	See the @ref channelbuffers "Channel Buffers" module for more details.  @ref tut-buffered "Buffered channels"
	*	are also covered in the guide.
	*
	*	@section other Other Channel Actions:
	*
	*	So far only reading and writing to a channel have been described.  There are three more 
	*	actions that can be performed on channels, and these are described in the following subsections.
	*
	*	@subsection ext Extended Inputs
	*
	*	Extended inputs, also known as extended rendezvous, are a way for a reader to perform an extended
	*	action during a communication.  @ref tut-extinput "Extended inputs" are covered in the guide.
	*	This facility is available on all unbuffered channels.  It is
	*	also available on buffered channels, although its effect may not be immediately obvious - see
	*	the @ref channelbuffers "Channel Buffers" module for more details on extended inputs on 
	*	buffered channels.
	*
	*	@subsection alting ALTing
	*
	*	ALTing (named for the occam construct) is a way to provide choice between many (ALTernatives).
	*	It can be thought of as bearing similarity to C++'s "switch" construct, although taking this as 
	*	a literal comparison may lead to confusion.
	*
	*	In the context of channels, ALTing is often used to read on either - say - channel A or channel B.
	*	In fact, the choice may consist of any number of channels, along with other possibilities 
	*	(such as timeouts).  The choice can be prioritised or fair, and more information is available on
	*	the documentation for the csp::Alternative class, and also in the @ref tut-alt "Alternative" section
	*	of the guide.
	*
	*	In the context of channels, a common question is why input guards are allowed (that is, ALTs
	*	can have a channel-read as an option) but output guards are not (that is, ALTs cannot have a
	*	channel-write as an option).
	*
	*	The answer to this is primarily implementation-related.  Currently C++CSP2 can assume that writers
	*	to a channel are always committed (that is, once they have attempted a communication, they will
	*	always complete it, and will not "back out"), whereas readers will not always be committed (if
	*	a reader is ALTing over a channel, it is offering to communicate but may end up choosing another
	*	ready guard rather than this channel, thus backing out of the communication).  This is not difficult
	*	to implement, but a system whereby both the reader and writer may back out of the communication
	*	at any time is much harder and more expensive (in terms of time and complexity) to implement.
	*
	*	Having said that, the new ALTing barrier mechanism recently introduced by Welch and Barnes (at CPA 2006)
	*	offers a way to implement output guards on specialised channels.  I hope to be able to include
	*	such channels (that allow output guards) in C++CSP2 in the future, along with ALTing barriers.
	*
	*	@subsection chanpoison Poison
	*	
	*	All C++CSP2 channels support poison.  Poison is a mechanism for shutting down 
	*	process networks.  More details can be found in the @ref poison "Poison" module, and in the
	*	@ref tut-channelpoison "Channel Poison" section of the guide.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	For One2OneChannel, One2AnyChannel, Any2OneChannel and Any2AnyChannel, DATA_TYPE* must be a valid type;
	*	DATA_TYPE cannot be a reference.  DATA_TYPE must allow assignment for these channels, but no restriction is
	*	placed on the constructors.
	*
	*	For BufferedOne2OneChannel, BufferedOne2AnyChannel, BufferedAny2OneChannel and BufferedAny2AnyChannel, 
	*	DATA_TYPE* must be a valid type (DATA_TYPE cannot be a reference).  No other restrictions are technically placed 
	*	by the channel; however, the @ref channelbuffers "Channel Buffer" will impose further restrictions, which will almost certainly require
	*	at least assignment of DATA_TYPE to be possible.
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	The channels themselves are unchanged from C++CSP v1.x.  The channel ends have been changed
	*	(Chanin has been split into Chanin and AltChanin - more details on the @ref channelends "Channel Ends" page),
	*	the channel buffers have changed (more details on the @ref channelbuffers "Channel Buffers" page),
	*	but the details provided here are the same as in version 1.x.
	*
	*	@{
	*/
	
	/** @class One2OneChannel
	*
	*	A one-to-one unbuffered channel.  Please read the @ref channels "Channels" section for more information on this class.
	*/
	
	/** @class Any2OneChannel
	*
	*	An any-to-one unbuffered channel.  Please read the @ref channels "Channels" section for more information on this class.
	*/
	
	/** @class One2AnyChannel
	*
	*	A one-to-any unbuffered channel.  Please read the @ref channels "Channels" section for more information on this class.
	*/
	
	/** @class Any2AnyChannel
	*
	*	An any-to-any unbuffered channel.  Please read the @ref channels "Channels" section for more information on this class.
	*/
	
	/** @class BufferedOne2OneChannel
	*
	*	A one-to-one buffered channel.  Please read the @ref channels "Channels" section for more information on the channel, or
	*	read the @ref channelbuffers "Channel Buffers" section for more information on the various buffering strategies.
	*/
	
	/** @class BufferedAny2OneChannel
	*
	*	A any-to-one buffered channel.  Please read the @ref channels "Channels" section for more information on the channel, or
	*	read the @ref channelbuffers "Channel Buffers" section for more information on the various buffering strategies.
	*/
	
	/** @class BufferedOne2AnyChannel
	*
	*	A one-to-any buffered channel.  Please read the @ref channels "Channels" section for more information on the channel, or
	*	read the @ref channelbuffers "Channel Buffers" section for more information on the various buffering strategies.
	*/
	
	/** @class BufferedAny2AnyChannel
	*
	*	An any-to-any buffered channel.  Please read the @ref channels "Channels" section for more information on the channel, or
	*	read the @ref channelbuffers "Channel Buffers" section for more information on the various buffering strategies.
	*/
	
	/** @class BlackHoleChannel
	*
	*	A "one-to-none" channel.  Sometimes when constructing a process network, you will find that 
	*	a process is writing to a channel where you have no need of that data.  In this case you
	*	effectively want to discard the data.  One way of doing this is to have a receiver process
	*	on the reading end of the channel that does nothing with the data.  However, this is wasteful -
	*	so this black-hole channel is provided for that use.  It is particularly useful for testing processes.
	*
	*	All data written to a black-hole channel - as its name suggests - is lost.  It should only
	*	be used by one writer.  The channel does support poison - although only the writer can poison it
	*	or detect the poison, so it is fairly useless.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE* must be a valid type; DATA_TYPE cannot be a reference.  No further restrictions are placed on DATA_TYPE.
	*/
	
	/** @class WhiteHoleChannel
	*
	*	A "none-to-one" channel.  This channel is the reverse of the BlackHoleChannel, as its name suggests.
	*	Where the BlackHoleChannel discards all data written to it, the WhiteHoleChannel forever
	*	produces the same piece of data for every read.  An ALT guard for the WhiteHoleChannel is always ready immediately.
	*
	*	The white-hole channel can be useful when you want a process to always be able to read from a particular
	*	channel.  It is particularly useful for testing processes.
	*
	*	It should only	be used by one reader.  The channel does support poison - although only the reader can poison it
	*	or detect the poison, so it is fairly useless.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE* and DATA_TYPE& must be valid types; DATA_TYPE cannot be a reference.  DATA_TYPE must have an available
	*	copy constructor, and must support assignment.
	*/
	
	/** @} */ //end of group
}
