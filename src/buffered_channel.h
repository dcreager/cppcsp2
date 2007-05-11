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
class BufferedChannelTest;
class AltChannelTest;

//NOTE: Much of the documentation for these classes is in the channel.h file

namespace csp
{	
	template <typename DATA_TYPE>
	class BufferedOne2OneChannel;

	template <typename DATA_TYPE>
	class BufferedOne2AnyChannel;

	template <typename DATA_TYPE>
	class BufferedAny2OneChannel;
	
	template <typename DATA_TYPE>
	class BufferedAny2AnyChannel;

	
	namespace internal
	{
		
	template <typename DATA_TYPE,typename MUTEX = internal::PureSpinMutex>
	class _BufferedOne2OneChannel
		:	protected internal::BaseAltChan<DATA_TYPE>, private internal::Primitive
	{
	private:
		MUTEX mutex;
		volatile internal::AltingProcessPtr waitingProcess;
		union
		{
			DATA_TYPE* volatile dest;
			const DATA_TYPE* volatile src;
		};
		using internal::BaseAltChan<DATA_TYPE>::isPoisoned;
		bool volatile * volatile commFinished;
		
		virtual void input(DATA_TYPE* const _dest)
		{
			typename MUTEX::End end(mutex.end());
			end.claim();
		
			if (false == buffer->inputWouldSucceed())
			{
				if (isPoisoned)
				{
					end.release();
					throw PoisonException();
				}
				
				//channel is empty, so we will wait:

				waitingProcess = currentProcess();
				dest = _dest;
				
				bool finished = false;
				commFinished = &finished;
				
				end.release();
				reschedule();
				//Communication done, or we were poisoned:				
				if (false == finished)
				{					
					throw PoisonException();
				}				
	        }
			else
			{
				//We don't check poison if there is data ready
			
				//get data from the buffer:
				buffer->get(_dest);

				//now see if there is an outputter waiting on the buffer:
				if (waitingProcess != NULL && buffer->outputWouldSucceed(src))
				{
					//there is, so do their transfer and wake them up
					buffer->put(src);
					
					*commFinished = true;

					internal::AltingProcessPtr wasWaiting = waitingProcess;
					waitingProcess = NULL;
					src = NULL;
					
					freeProcessNoAlt(wasWaiting);
				}
				
				end.release();
			}		
		}		
		
		virtual void beginExtInput(DATA_TYPE* const _dest)
		{
			typename MUTEX::End end(mutex.end());
			end.claim();
		
			if (false == buffer->inputWouldSucceed())
			{
				if (isPoisoned)
				{
					end.release();
					throw PoisonException();
				}
				
				//channel is empty, so we will wait:

				waitingProcess = currentProcess();
				dest = NULL;
				
				bool finished = false;
				commFinished = &finished;
				
				end.release();
				reschedule();
				//Communication done, or we were poisoned:
								
				if (false == finished)
				{				
					throw PoisonException();
				}
				
				end.claim();
				
				//This makes the semantics slightly interesting.  If a writer writes to an overwriting buffer
				//and then writes over all the data including the original one, it will be lost even though
				//it was the first data written to an empty buffer.  I don't think this breaks any expected semantics
				buffer->beginExtGet(_dest);
				
				end.release();
	        }
			else
			{
				//We don't check poison if there is data ready
			
				//get data from the buffer:
				buffer->beginExtGet(_dest);
								
				end.release();
			}		
		}
		virtual void endExtInput()
		{
			typename MUTEX::End end(mutex.end());
			end.claim();
			
			buffer->endExtGet();
			
				//now see if there is an outputter waiting on the buffer:
				if (waitingProcess != NULL && buffer->outputWouldSucceed(src))
				{
					//there is, so do their transfer and wake them up
					buffer->put(src);
					
					*commFinished = true;

					internal::AltingProcessPtr wasWaiting = waitingProcess;
					waitingProcess = NULL;
					src = NULL;
					
					freeProcessNoAlt(wasWaiting);
				}
			
			end.release();
		}
		virtual void output(const DATA_TYPE* const _src)
		{								
			typename MUTEX::End end(mutex.end());
			end.claim();
			
			if (isPoisoned)
			{
				end.release();
				throw PoisonException();
			}
			
			if (false == buffer->outputWouldSucceed(_src))
			{
				//we'll have to wait
				waitingProcess = currentProcess();
				src = _src;
				
				bool finished = false;
				commFinished = &finished;
				
				end.release();
				reschedule();
				//Either communication has finished, or we were poisoned:				
				if (false == finished)
				{					
					throw PoisonException();
				}				
			}
			else
			{
				buffer->put(_src);
				//now see if there is a reader waiting on the buffer:
				if (waitingProcess != NULL)
				{
					//there is, so do their transfer and wake them up
					
					if (dest == NULL || buffer->inputWouldSucceed())
					{
						if (dest != NULL)
						{
							buffer->get(dest);
						}
						
						*commFinished = true;
																							
						internal::AltingProcessPtr wasWaiting = waitingProcess;
						waitingProcess = NULL;
						dest = NULL;
					
						//It is important to free the process before releasing the mutex.
						//Otherwise, there is a potential race hazard whereby an outputter can be ready to free an alter
						//but gets scheduled out.  The alter comes in, disables successfully (because it can claim the
						//mutex) and moves on.  Then this process frees it, causing it to be added to the run-queue spuriously!
					
						freeProcessMaybe(wasWaiting);
					}
				}
				end.release();
			}
		}	
				
		inline void _poison(bool clearBuffer)
		{
			typename MUTEX::End end(mutex.end());
			end.claim();
		
			isPoisoned = true;
			
			if (clearBuffer)
			{
				buffer->clear();
			}
			
			if (waitingProcess != NullProcessPtr)
			{
				internal::AltingProcessPtr wasWaiting = waitingProcess;
				waitingProcess = NULL;
					
				freeProcessMaybe(wasWaiting);
			}						
			end.release();
		}
		
		virtual void poisonIn()
		{
			_poison(true);
		}
		
		virtual void poisonOut()
		{
			_poison(false);
		}
		
		class __ChannelGuard : public csp::Guard
		{
			volatile bool finished;
			_BufferedOne2OneChannel<DATA_TYPE>* channel;
		public:
			inline __ChannelGuard(_BufferedOne2OneChannel<DATA_TYPE>* _channel)
				:	channel(_channel)
			{
			}
		
			inline bool enable(ProcessPtr proc)
			{
				typename MUTEX::End end(channel->mutex.end());
				end.claim();
				bool ret = false;
				
					if (channel->buffer->inputWouldSucceed() || channel->isPoisoned)
					{
						ret = true;
					}
					else
					{
						//We need to put ourselves in the channel:
						channel->dest = NULL;
						channel->waitingProcess = proc;
						//To keep the pointer valid:
						channel->commFinished = &finished;
						ret = false;
					}
								
				end.release();
				
				return ret;
			}
			
			inline bool disable(ProcessPtr proc)
			{
				typename MUTEX::End end(channel->mutex.end());
				end.claim();
				bool ret = false;
				
					if (channel->waitingProcess != proc || channel->buffer->inputWouldSucceed() || channel->isPoisoned)
					{
						//Someone else is in the channel or its poisoned:
						ret = true;
					}
					else
					{
						channel->waitingProcess = NullProcessPtr;
						ret = false;
					}
				end.release();
				
				return ret;
			}
		};
		
		virtual Guard* inputGuard()
		{
			return new __ChannelGuard(this);
		}		

		/**@internal
		*	Checks whether an input would succeed immediately on the channel
		*
		*	This is used as a short form of polling a channel
		*	(that would otherwise have to be done using a <tt>PRI ALT</tt>)
		*
		*	@return Whether an input would succeed immediately on this channel
		*/
		virtual bool pending()
		{
			typename MUTEX::End end(mutex.end());
			end.claim();		
		
				bool ret;
				ret = buffer->inputWouldSucceed() || isPoisoned;
			
			end.release();
			return ret;
		}

	protected:
		ChannelBuffer<DATA_TYPE>* buffer;	

		//For the child Buffered.. channels
		_BufferedOne2OneChannel()
			:	waitingProcess(NULL),
				buffer(NULL)
		{
			dest = NULL;
			src = NULL; //both, just in case
		}

	
	public:
		_BufferedOne2OneChannel(const ChannelBufferFactory<DATA_TYPE>& bufferFactory)
			:	waitingProcess(NULL),
				buffer(bufferFactory.createBuffer())
		{
			dest = NULL;
			src = NULL; //both, just in case
		}		
		
		/**
		*	Destructor
		*/
		virtual ~_BufferedOne2OneChannel()
		{
			delete buffer;
		}

		/**
		*	Gets a reading end for the channel
		*
		*	@see Chanin
		*/
		inline AltChanin<DATA_TYPE> reader()
		{
			return AltChanin<DATA_TYPE>(this,true);
		}		

		/**
		*	Gets a writing end for the channel
		*
		*	@see Chanout
		*/
		inline Chanout<DATA_TYPE> writer()
		{
			return Chanout<DATA_TYPE>(this,true);
		}		
		
		
		template <typename CHANNEL,typename _DATA_TYPE,typename _MUTEX>
		friend class internal::Any2OneAdapter;
		
		template <typename CHANNEL,typename _DATA_TYPE,typename _MUTEX>
		friend class internal::One2AnyAdapter;
		
		template <typename CHANNEL,typename _DATA_TYPE>
		friend class internal::Any2AnyAdapter;		
		
		
		friend class csp::BufferedOne2OneChannel<DATA_TYPE>;
				
		friend class csp::BufferedOne2AnyChannel<DATA_TYPE>;
		
		friend class csp::BufferedAny2OneChannel<DATA_TYPE>;
			
		friend class csp::BufferedAny2AnyChannel<DATA_TYPE>;
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::BufferedChannelTest;
		friend class ::AltChannelTest;
		
	}; //class _BufferedOne2OneChanel
	
	} //namespace internal

	template <typename DATA_TYPE>
	class BufferedOne2OneChannel : public internal::_BufferedOne2OneChannel<DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
	protected:
		//For the any2 and 2any channels:
		BufferedOne2OneChannel()
		{
		}
			
	public:		
		BufferedOne2OneChannel(const ChannelBufferFactory<DATA_TYPE>& bufferFactory)
			:	internal::_BufferedOne2OneChannel<DATA_TYPE>(bufferFactory)
		{			
		}
		
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
		
		friend class csp::BufferedOne2AnyChannel<DATA_TYPE>;
		
		friend class csp::BufferedAny2OneChannel<DATA_TYPE>;
			
		friend class csp::BufferedAny2AnyChannel<DATA_TYPE>;		

		//For testing:
		friend class ::BufferedChannelTest;
		friend class ::ChannelTest;
		friend class ::AltChannelTest;
		
	};
	
	template <typename DATA_TYPE>
	class BufferedAny2OneChannel : public internal::Any2OneAdapter<BufferedOne2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{		
	public:
		BufferedAny2OneChannel(const ChannelBufferFactory<DATA_TYPE>& bufferFactory)			
		{
			internal::_BufferedOne2OneChannel<DATA_TYPE>::buffer = bufferFactory.createBuffer();
		}
	
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		AltChanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::BufferedChannelTest;
		friend class ::AltChannelTest;
		
	};
	
	
	template <typename DATA_TYPE>
	class BufferedOne2AnyChannel : public internal::One2AnyAdapter<BufferedOne2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
	public:
		BufferedOne2AnyChannel(const ChannelBufferFactory<DATA_TYPE>& bufferFactory)			
		{
			BufferedOne2OneChannel<DATA_TYPE>::buffer = bufferFactory.createBuffer();
		}
		
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		Chanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::BufferedChannelTest;
		friend class ::AltChannelTest;
	};
	
	template <typename DATA_TYPE>
	class BufferedAny2AnyChannel : public internal::Any2AnyAdapter<BufferedOne2OneChannel<DATA_TYPE>,DATA_TYPE>
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
	public:
		BufferedAny2AnyChannel(const ChannelBufferFactory<DATA_TYPE>& bufferFactory)			
		{
			internal::_BufferedOne2OneChannel<DATA_TYPE>::buffer = bufferFactory.createBuffer();
		}
		
		#ifdef CPPCSP_DOXYGEN
		///Gets the reading end of the channel
		Chanin<DATA_TYPE> reader();		
		
		///Gets the writing end of the channel
		Chanout<DATA_TYPE> writer();
		#endif	
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::BufferedChannelTest;
		friend class ::AltChannelTest;
	};


} // namespace csp
