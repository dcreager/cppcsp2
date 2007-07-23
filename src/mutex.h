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

#include <boost/config.hpp>

#include <iostream>

#include <map>

#ifdef CPPCSP_POSIX
	#include <semaphore.h>
	#include <errno.h>
#endif

namespace csp
{
	namespace internal
	{		
		/**@internal
		*	@defgroup mutex Mutexes
		*
		*	Internally, C++CSP2 uses mutexes for many of its algorithms.  Having an explicit mutex
		*	base class did not make sense, for two reasons.  First - the cost of virtual function calls
		*	would be unnecessary (it can be avoided by using a policy template argument to the appropriate
		*	classes) and secondly it did not fit exactly because some mutexes (such as QueuedMutex) need
		*	to retain data in each mutex end whereas others (such as PureSpinMutex) do not.
		*
		*	Therefore, as in boost's threading library, a mutex is a class that follows a certain model,
		*	rather than having a base class.
		*
		*	A given MUTEX must have a default (no arguments) constructor, and a function called end() that returns
		*	something of type MUTEX::End.  A MUTEX::End must have a claim() function and a release() function.
		*	It does not need to have a default constructor.  For the simpler mutexes, typedefing MUTEX& as MUTEX::End
		*	is an easy way to meet these requirements, including the claim() and release() functions as part
		*	of the MUTEX class.
		*
		*	On most, if not all, of the mutexes claiming twice without a release or releasing without a claim
		*	will result in undefined behaviour.  Since the uses of these mutexes is internal to C++CSP2, it
		*	will be enough that the internal algorithms are correct to prevent any problems.
		*	MUTEXs are not expected to be copyable - in fact, they usually inherit from boost::noncopyable.		
		*	None of the mutexes have a limit on the number of mutex ends that may be held.
		*
		*	@{
		*/
	
		/**@internal
		*	A simple null-mutex.  This mutex is inherently dangerous (because it is not really a mutex)
		*	but it is provided for some benchmarks that want to see how fast various C++CSP2 primitives
		*	would be without a mutex.  Do not use this mutex unless you are sure of what you are doing -
		*	it is only usually safe with a primitive (such as a channel or barrier) if all the processes
		*	using the primitive are always within the same thread, and not always then (e.g. using this with
		*	a shared channel-end will lead to disaster).
		*/
		class NullMutex
		{
		public:
			inline void claim() {}
			inline void release() {}
		
			typedef NullMutex& End;
		
			End end() {return *this; }
		};
	

#ifdef CPPCSP_ATOMICS
		/**@internal 
		*	A mutex that can be used across multiple-threads, and by multiple processes in the same thread, without blocking the whole thread.
		*	No spinning is performed during the claim, although there is a remote possibility of a short spin in the release(), strangely!
		*	FIFO ordering of claimers is guaranteed.  
		*
		*	This mutex must NOT be used, ever, by a process that is ALTing.  Use one of the spin mutexes instead in that case.
		*
		*	This mutex is an implementation of the MCS (Mellor-Crummey Scott) mutex algorithm, but with the spinning
		*	in that algorithm translated into a C++CSP2 block-and-reschedule here.
		*/
		class QueuedMutex : private internal::Primitive, public boost::noncopyable
		{
		private:
			/**
			*	A link in the mutex chain.  Simply has a process pointer to the claimer,
			*	and an aligned pointer to the next link in the chain
			*/
			class Link
			{
			public:
				ProcessPtr process;
				__CPPCSP_ALIGNED_PTR(Link) link;
			};
			
			/**
			*	An aligned pointer that points to the last link in the chain.  NULL means that the chain is empty, and the mutex is unclaimed.
			*/
			__CPPCSP_ALIGNED_PTR(Link) queueTail;
			
			
			
		public:
			/**
			*	The key type that is held by each mutex end.  Key is simply equivalent to Link
			*/
			typedef Link Key;
			
			/**
			*	Constructs the mutex in an unclaimed state
			*/
			inline QueuedMutex()
				:	queueTail(NULL)
			{
			}
		
			/**
			*	Claims the mutex.  
			*
			*	If the mutex is currently unclaimed, the call will return without blocking.  If the mutex
			*	was already claimed, the process will queue up, and then reschedule itself.  It will only
			*	be released and let this call return once it has claimed the mutex.
			*						
			*	Calling claim() twice without calling release() will result in undefined behaviour.
			*/
			inline void claim(Key* key)
			{
				key->process = currentProcess();
				key->link = NULL;
				
				Link* oldTail = AtomicSwap(&queueTail,key);
				
				if (oldTail == NULL)
				{
					//We have the lock now
				}
				else
				{
					AtomicPut(&(oldTail->link),key);
					
					reschedule();
					//Now we have the lock
				}
			}
			
			inline bool isClaimed()
			{
				return NULL != AtomicGet(&queueTail);
			}
			
			
			/**
			*	Releases the mutex.
			*
			*	This operation will not block.  However, if the release happens simultaneously to another
			*	process joining the queue as the immediate-next process, there may be some spinning before
			*	this call returns.  This will be a rare occurrence.
			*
			*	Calling release() when you have not claimed the mutex will result in undefined behaviour.
			*/
			inline void release(Key* key)
			{
				Link* keyLink;
				
				//Check if there is someone after us in the queue 
				if ((keyLink = AtomicGet(&(key->link))) == NULL)
				{
					//Doesn't appear to be anyone, let's try swapping us out from the mutex:				
					if (AtomicCompareAndSwap<Link>(&queueTail,key,NULL) == key)
					{
						//No-one else arrived and queued up in the mean-time, done
						return;
					}
					else
					{
						int spinCount = 0;
						//Someone is in the middle of adding themselves.  Spin until they have:
						while ((keyLink = AtomicGet(&(key->link))) == NULL)
						{
							spin(spinCount++);
						}
					}
				}
				//Now, key->link != NULL:
				
				//Merely waking them up grants them the lock:
				freeProcessNoAlt(keyLink->process);
			}
			
			/**@internal
			*	The end of a queued mutex
			*/
			class End
			{
			private:
				QueuedMutex* mutex;
				QueuedMutex::Key key;
				inline End(QueuedMutex* _mutex)
					:	mutex(_mutex)
				{
				}
			public:
				///Claims the mutex.  Will not return until the mutex is claimed.  @see QueuedMutex::claim(Key*)
				inline void claim()
				{
					mutex->claim(&key);
				}
				
				///Releases the mutex. @see QueuedMutex::release(Key*)
				inline void release()
				{
					mutex->release(&key);
				}
				
				///QueuedMutex is a friend so that it can access the private constructor
				friend class QueuedMutex;
			};
			
			///Provides an end of the mutex.  
			End end()
			{
				return End(this);
			}
		};		


		/**@internal
		*	A pure-spinning mutex that does not interact with the C++CSP2 kernel.
		*
		*	This mutex uses atomic operations to implement a mutex.  If a claim is initially unsuccessful,
		*	the mutex spins (using Primitive::spin() ) until the mutex is claimed.
		*
		*	AtomicCompareAndSwap is used for claiming, AtomicPut is used for release.
		*/
		class PureSpinMutex : private internal::Primitive, public boost::noncopyable
		{
		private:
			/**
			*	The value used to represent whether or not the mutex is currently held.  NULL means free, any other value means claimed
			*/
			__CPPCSP_ALIGNED_VOID_PTR value;
		public:
			/**
			*	Constructs the mutex in an unclaimed state
			*/
			inline PureSpinMutex()
				:	value(NULL)
			{
			}
		
			/**
			*	Tries to claim the mutex once, without spinning. 
			*
			*	@return True if the mutex was claimed, false if it was not
			*/
			inline bool tryClaim();
			
			/**
			*	Claims the mutex.  This call will only return once the mutex is claimed.
			*
			*	The spinning policy is defined by Primitive::spin()
			*
			*	Claiming the mutex twice without a release will lead to an infinite loop.
			*/
			inline void claim()
			{
				int spinCount = 0;
				while (false == tryClaim())
				{
					spin(++spinCount);
				}
			}
			
			inline bool isClaimed()
			{
				return NULL != AtomicGet(&value);
			}
			
			//Deliberately undocumented feature, until I find a use for it
			static std::vector<bool> TryClaimAny(std::list<PureSpinMutex*>);
			
			/**
			*	Releases the mutex.
			*
			*	This call always returns immediately, without any possibility of spinning.
			*
			*	Calling release without claiming the mutex will result in undefined behaviour
			*/
			inline void release();
			
			/**
			*	The end of a pure-spin mutex
			*/
			typedef PureSpinMutex& End;
			
			/**
			*	Gets an end of the mutex.
			*/
			End end()
			{
				return *this;
			}
		};
		
		/**@internal
		*	A pure-spinning test-and-test-and-set mutex that does not interact with the C++CSP2 kernel.
		*
		*	This mutex uses atomic operations to implement a mutex.  If a claim is initially unsuccessful,
		*	the mutex spins (using Primitive::spin() ) until the mutex is claimed.  
		*
		*	The mutex is very similar to PureSpinMutex, but uses a test-and-test-and-set policy for claiming.
		*	That is, AtomicGet is used first, as a read-only check for whether the mutex is free.  If the mutex
		*	is free, a claim is attempted using AtomicCompareAndSwap.  If either of these is unsuccessful,
		*	the procedure is repeated with AtomicGet, spinning until the mutex is claimed.  
		*	The spin count passed to Primitive::spin() is increment on each AtomicGet. AtomicPut is
		*	used for release.
		*/
		class PureSpinMutex_TTS : private internal::Primitive, public boost::noncopyable
		{
		private:
			/**
			*	The value used to represent whether or not the mutex is currently held.  NULL means free, any other value means claimed
			*/
			__CPPCSP_ALIGNED_VOID_PTR value;
		public:
			/**
			*	Constructs the mutex in an unclaimed state
			*/
			inline PureSpinMutex_TTS()
				:	value(NULL)
			{
			}
		
			/**
			*	Tries to claim the mutex once, without spinning. 
			*
			*	@return True if the mutex was claimed, false if it was not
			*/
			inline bool tryClaim();
			
			/**
			*	Claims the mutex.  This call will only return once the mutex is claimed.
			*
			*	The spinning policy is defined by Primitive::spin()
			*
			*	Claiming the mutex twice without a release will lead to an infinite loop.
			*/
			inline void claim()
			{
				int spinCount = 0;
				while (false == tryClaim())
				{
					spin(++spinCount);
				}
			}			
			
			/**
			*	Releases the mutex.
			*
			*	This call always returns immediately, without any possibility of spinning.
			*
			*	Calling release without claiming the mutex will result in undefined behaviour
			*/
			inline void release();
			
			/**
			*	The end of a pure-spin TTS mutex
			*/
			typedef PureSpinMutex_TTS& End;
			
			/**
			*	Gets an end of the mutex.
			*/
			End end()
			{
				return *this;
			}
		};
		
		/**@internal
		*	A spin mutex that spins intelligently, possibly using the C++CSP2 kernel.
		*
		*	This mutex behaves like a pure-spin mutex, but varies its spinning tactic.  If the 
		*	mutex is held by another process in the same thread, it performs a C++CSP2-yield (without spinning furhter), which
		*	should hopefully allow the user of the mutex to finish with it.  If the mutex is held by
		*	another thread, it uses Primitive::spin(), just like the PureSpinMutex and PureSpinMutex_TTS.
		*
		*	So far, I have yet to find a use for this mutex inside the library.  Where mutexes will not be
		*	held over a kernel-reschedule, this mutex is unnecessary (as it is inherent that a claimed mutex is
		*	being held by another thread).  Where mutexes will be held for a long period of time, a queued mutex
		*	is more appropriate to remove spinning.  Currently, there is no situation will a mutex will be held for 
		*	a short period of time.  However, this mutex is left in for when that may be the case, and it is also
		*	interesting in benchmarks.
		*/
		class SpinMutex : private internal::Primitive, public boost::noncopyable
		{
		private:
			/**
			*	The value used to represent whether or not the mutex is currently held.  NULL means free, 
			*	any other value means claimed, and the pointer is to the kernel for the thread of the claimer.
			*/
			__CPPCSP_ALIGNED_PTR(Kernel) value;
		public:
			/**
			*	Constructs the mutex in an unclaimed state
			*/
			inline SpinMutex()
				:	value(NULL)
			{
			}
		
			/**
			*	Tries to claim the mutex once, without spinning. 
			*
			*	@param kernelClaimer The kernel of the current holder of the mutex
			*	@return True if the mutex was claimed, false if it was not
			*/
			inline bool tryClaim(Kernel** kernelClaimer);
			
			/**
			*	Claims the mutex.  This call will only return once the mutex is claimed.
			*
			*	The spinning policy is defined by Primitive::spin() if the mutex is held by
			*	a process in another thread, or simply uses CPPCSP_Yield if the mutex is 
			*	held by a process in this thread
			*
			*	Claiming the mutex twice without a release will lead to an infinite loop.
			*/
			inline void claim()
			{
				Kernel* p;				
				if (false == tryClaim(&p))
				{
					if (p == GetKernel())
					{
						do
						{
							CPPCSP_Yield();
						}
						while (false == tryClaim(&p));
					}
					else
					{					
						int spinCount = 0;
						do
						{
							spin(++spinCount);
						}
						while (false == tryClaim(&p));
					}
				}
			}			
			
			/**
			*	Releases the mutex.
			*
			*	This call always returns immediately, without any possibility of spinning.
			*
			*	Calling release without claiming the mutex will result in undefined behaviour
			*/
			inline void release();
			
			/**
			*	The end of a spin mutex
			*/
			typedef SpinMutex& End;
			
			/**
			*	Gets an end of the mutex.
			*/
			End end()
			{
				return *this;
			}
		};

#endif //CPPCSP_ATOMICS
		
		/**@internal
		*	A class that wraps an operating system mutex into our mutex model.  This mutex blocks the whole thread!
		*	It is therefore very ill-advised in the context of a C++CSP2 program.
		*
		*	On Windows, it uses CreateMutex, etc.  On pthreads it uses pthread mutexes.  There is no
		*	interaction with the C++CSP2 kernel.
		*
		*	This mutex is used only for benchmarking.
		*/
		class OSBlockingMutex : public boost::noncopyable
		{
		#ifdef CPPCSP_WINDOWS
			HANDLE mutex;
		#else
			pthread_mutex_t mutex;			
		#endif
		public:
			/**
			*	Claims the mutex.  This claim operation blocks the whole thread until it is complete.			
			*/
			inline void claim()
			{
			#ifdef CPPCSP_WINDOWS
				if (WAIT_OBJECT_0 != WaitForSingleObject(mutex,INFINITE))
				{
					throw CPPCSPError("Mutex not claimed");
				}
			#else
				pthread_mutex_lock(&mutex);
			#endif
			}
		
			/**
			*	Releases the mutex.  This operation should not block or spin.
			*/
			inline void release()
			{
			#ifdef CPPCSP_WINDOWS
				ReleaseMutex(mutex);
			#else
				pthread_mutex_unlock(&mutex);
			#endif
			}
			
			/**
			*	Constructs the mutex in an unclaimed state.
			*/
			inline OSBlockingMutex()
			{
			#ifdef CPPCSP_WINDOWS
				mutex = CreateMutex(NULL,FALSE,NULL);
			#else
				pthread_mutex_init(&mutex,NULL);
			#endif
			}
			
			inline ~OSBlockingMutex()
			{
			#ifdef CPPCSP_WINDOWS
				CloseHandle(mutex);
			#else
				pthread_mutex_destroy(&mutex);
			#endif
			}
			
			/**
			*	The end of a blocking OS mutex
			*/
			typedef OSBlockingMutex& End;
			
			/**
			*	Gets an end of the mutex.
			*/
			End end()
			{
				return *this;
			}
		};
		
		//TODO later benchmark altering the spin count
		
		#ifdef CPPCSP_WINDOWS
		//TODO later document this class
		class OSCritSection : public boost::noncopyable
		{
		private:
			CRITICAL_SECTION crit;
		public:
			inline OSCritSection()
			{
				InitializeCriticalSection(&crit);
				SetCriticalSectionSpinCount(&crit,3);
			}
			
			inline ~OSCritSection()
			{
				DeleteCriticalSection(&crit);
			}
			
			void claim()
			{
				EnterCriticalSection(&crit);
			}
			
			void release()
			{
				LeaveCriticalSection(&crit);
			}
					
			typedef OSCritSection& End;
			End end()
			{
				return *this;
			}
		};
		
		class OSNonBlockingCritSection : public boost::noncopyable, private internal::Primitive
		{
		private:
			CRITICAL_SECTION crit;
		public:
			inline OSNonBlockingCritSection()
			{
				InitializeCriticalSection(&crit);
				SetCriticalSectionSpinCount(&crit,3);
			}
			
			inline ~OSNonBlockingCritSection()
			{
				DeleteCriticalSection(&crit);
			}
			
			bool tryClaim()
			{
				return (0 != TryEnterCriticalSection(&crit));
			}
			
			void claim()
			{
				int spinCount = 0;
				while (false == tryClaim())
				{
					spin(++spinCount);
				}				
			}
			
			void release()
			{
				LeaveCriticalSection(&crit);
			}
					
			typedef OSNonBlockingCritSection& End;
			End end()
			{
				return *this;
			}
		};
		
		#endif //CPPCSP_WINDOWS
		
		
		/**@internal
		*	A class that wraps an operating system mutex into our mutex model.
		*
		*	On Windows, it uses CreateMutex, etc.  On pthreads it uses pthread mutexes.  There is no
		*	interaction with the C++CSP2 kernel.
		*
		*	Currently, this mutex is used only for benchmarking.
		*/
		class OSNonBlockingMutex : private internal::Primitive,boost::noncopyable
		{
		#ifdef CPPCSP_WINDOWS
			HANDLE mutex;
		#else
			pthread_mutex_t mutex;			
		#endif
		public:
			/**
			*	Claims the mutex.  If the claim is unsuccessful, the mutex spins using
			*	Primitive::spin() until the claim is successful.
			*/
			inline void claim()
			{
				int spinCount = 0;
				while
			#ifdef CPPCSP_WINDOWS
				(WAIT_OBJECT_0 != WaitForSingleObject(mutex,0))
			#else
				(0 != pthread_mutex_trylock(&mutex))
			#endif
				{
					spin(++spinCount);
				}
			}
		
			/**
			*	Releases the mutex.  This operation should not block or spin.
			*/
			inline void release()
			{
			#ifdef CPPCSP_WINDOWS
				ReleaseMutex(mutex);
			#else
				pthread_mutex_unlock(&mutex);
			#endif
			}
			
			/**
			*	Constructs the mutex in an unclaimed state.
			*/
			inline OSNonBlockingMutex()
			{
			#ifdef CPPCSP_WINDOWS
				mutex = CreateMutex(NULL,FALSE,NULL);
			#else
				pthread_mutex_init(&mutex,NULL);
			#endif
			}
			
			inline ~OSNonBlockingMutex()
			{
			#ifdef CPPCSP_WINDOWS
				CloseHandle(mutex);
			#else
				pthread_mutex_destroy(&mutex);
			#endif
			}
			
			/**
			*	The end of a blocking OS mutex
			*/
			typedef OSNonBlockingMutex& End;
			
			/**
			*	Gets an end of the mutex.
			*/
			End end()
			{
				return *this;
			}
		};
		
		template <typename MUTEX, unsigned CONDITIONS = 1>
		class MutexAndEvent : public boost::noncopyable
		{
		private:
			#ifdef CPPCSP_WINDOWS
				typedef HANDLE Event;
			#else
				typedef sem_t Event;
			#endif
		
			MUTEX mutex;
			Event events [CONDITIONS];
			
			char processIsWaiting [CONDITIONS];
			
		public:
			inline MutexAndEvent()
			{
				for (unsigned i = 0;i < CONDITIONS;i++)
				{
					#ifdef CPPCSP_WINDOWS
						events[i] = CreateEvent(NULL,FALSE,FALSE,NULL);					
					#else
						sem_init(&(events[i]),0,0);
					#endif
					
					processIsWaiting[i] = 0;
				}
				
				
			}
			
			inline ~MutexAndEvent()
			{
				for (unsigned i = 0;i < CONDITIONS;i++)
				{
					#ifdef CPPCSP_WINDOWS			
						CloseHandle(events[i]);					
					#else
						sem_destroy(&(events[i]));
					#endif
				}
			}
		
			inline void claim()
			{
				mutex.claim();
			}
			inline void release()
			{
				mutex.release();
			}			
			
			inline bool releaseWait(const csp::Time* timeout = NULL,unsigned cond = 0,__CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				#ifdef CPPCSP_WINDOWS
					ResetEvent(events[cond]);
				#else
					int val;
					sem_getvalue(&(events[cond]),&val);
					while (val > 0)
					{
						sem_wait(&(events[cond]));
						sem_getvalue(&(events[cond]),&val);
					}
				#endif
			
				processIsWaiting[cond] = (timeout == NULL) ? 1 : 2;
				mutex.release();
				#ifdef CPPCSP_WINDOWS
					if (timeout == NULL)
					{
						if (threadsRunningCount != NULL)
						{
							if (0 == AtomicDecrement(threadsRunningCount))
							{
								throw csp::internal::DeadlockException();
							}
						}
						WaitForSingleObject(events[cond],INFINITE);
					}
					else
					{
						csp::Time t;
						CurrentTime(&t);
						t = *timeout - t;
						sign32 millis = GetMilliSeconds(t);
						if (millis < 0 || WAIT_OBJECT_0 != WaitForSingleObject(events[cond],static_cast<DWORD>(millis)))
						{
							return false;
						}
					}
				#else
					if (timeout == NULL)
					{
						if (threadsRunningCount != NULL)
						{
							if (0 == AtomicDecrement(threadsRunningCount))
							{
								throw csp::internal::DeadlockException();
							}
						}
						while ( -1 == sem_wait(&(events[cond])) )
						{
							//std::cout << std::endl << "SEMAPHORE ERROR: " << errno << " (EINTR is: " << EINTR << ") " << std::endl << std::endl;
						}
					}
					else
					{
						timespec spec;
						spec.tv_sec = timeout->tv_sec;
						spec.tv_nsec = timeout->tv_usec * 1000;
						if (0 != sem_timedwait(&(events[cond]),&spec))
						{
							return false;
						}
					}
				#endif
				return true;			
			}
			
			///Timeout is absolute
			///If the timeout elapses, false is returned and the mutex is unclaimed
			inline bool releaseWaitClaim(const csp::Time* timeout = NULL,unsigned cond = 0, __CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				if (releaseWait(timeout,cond,threadsRunningCount))
				{
					mutex.claim();
					return true;
				}
				else
				{
					return false;
				}
			}
			
			
			inline void signal(unsigned cond = 0,__CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				if (processIsWaiting[cond] != 0)
				{
					if (processIsWaiting[cond] == 1 && threadsRunningCount != NULL)
					{
						AtomicIncrement(threadsRunningCount);
					}
					processIsWaiting[cond] = 0;
					#ifdef CPPCSP_WINDOWS
						SetEvent(events[cond]);
					#else
						sem_post(&(events[cond]));
					#endif
				}
			}			
		};
		
		template <unsigned CONDITIONS = 1>
		class Condition : public boost::noncopyable
		{
		private:
		#ifdef CPPCSP_WINDOWS
			HANDLE mutex;
			typedef HANDLE Event;
		#else
			pthread_mutex_t mutex;
			typedef pthread_cond_t Event;
		#endif
		
			Event events [CONDITIONS];
			
			char processIsWaiting [CONDITIONS];
		public:
			inline Condition()
			{
				#ifdef CPPCSP_WINDOWS	
					for (unsigned i = 0;i < CONDITIONS;i++)		
					{
						events[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
						processIsWaiting[i] = 0;
					}
					mutex = CreateMutex(NULL,FALSE,NULL);
				#else
					for (unsigned i = 0;i < CONDITIONS;i++)		
					{	
						pthread_cond_init(&(events[i]),NULL);
						processIsWaiting[i] = 0;
					}

					pthread_mutex_init(&mutex,NULL);

				#endif
				
				
			}
			
			inline ~Condition()
			{
				#ifdef CPPCSP_WINDOWS
					for (unsigned i = 0;i < CONDITIONS;i++)
					{
						CloseHandle(events[i]);
					}
					CloseHandle(mutex);
				#else
					for (unsigned i = 0;i < CONDITIONS;i++)
					{
						pthread_cond_destroy(&(events[i]));
					}
					
					pthread_mutex_destroy(&mutex);
				#endif
			}
			
			inline void claim()
			{
				#ifdef CPPCSP_WINDOWS
					WaitForSingleObject(mutex,INFINITE);
				#else
					pthread_mutex_lock(&mutex);
				#endif
			}
			
			inline void release()
			{
				#ifdef CPPCSP_WINDOWS
					ReleaseMutex(mutex);
				#else
					pthread_mutex_unlock(&mutex);
				#endif
			}
			
			inline bool releaseWaitClaim(csp::Time* timeout = NULL,unsigned cond = 0,__CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				processIsWaiting[cond] = (timeout == NULL) ? 1 : 2;
				#ifdef CPPCSP_WINDOWS
					ResetEvent(events[cond]);
					if (timeout == NULL)
					{
						if (threadsRunningCount != NULL)
						{
							if (0 == AtomicDecrement(threadsRunningCount))
							{
								throw csp::internal::DeadlockException();
							}
						}
						SignalObjectAndWait(mutex,events[cond],INFINITE,FALSE);
						WaitForSingleObject(mutex,INFINITE);
					}
					else
					{
						csp::Time t;
						CurrentTime(&t);
						t = *timeout - t;
						sign32 millis = GetMilliSeconds(t);
						if (millis < 0 || WAIT_OBJECT_0 != SignalObjectAndWait(mutex,events[cond],static_cast<DWORD>(millis),FALSE))
						{
							return false;
						}
						CurrentTime(&t);
						t = *timeout - t;
						millis = GetMilliSeconds(t);
						if (millis < 0 || WAIT_OBJECT_0 != WaitForSingleObject(mutex,static_cast<DWORD>(millis)))
						{
							return false;
						}
					}
				#else
					if (timeout == NULL)
					{
						if (threadsRunningCount != NULL)
						{
							if (0 == AtomicDecrement(threadsRunningCount))
							{
								throw csp::internal::DeadlockException();
							}
						}
					}
					//Because pthread_cond_wait and pthread_cond_timedwait
					//can spuriously wake-up, we need this while loop:										
					while (processIsWaiting[cond] != 0)
					{
						if (timeout == NULL)
						{
							pthread_cond_wait(&(events[cond]),&mutex);
						}
						else
						{
							timespec spec;
							spec.tv_sec = timeout->tv_sec;
							spec.tv_nsec = timeout->tv_usec * 1000;

							if (0 != pthread_cond_timedwait(&(events[cond]),&mutex,&spec))
							{
								release();
								return false;
							}
							
						}
					}
				#endif
				return true;
			}
			
			inline bool releaseWait(csp::Time* timeout = NULL,unsigned cond = 0,__CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				processIsWaiting[cond] = (timeout == NULL) ? 1 : 2;
				#ifdef CPPCSP_WINDOWS
					ResetEvent(events[cond]);
					if (timeout == NULL)
					{
						AtomicDecrement(threadsRunningCount);
						SignalObjectAndWait(mutex,events[cond],INFINITE,FALSE);					
					}
					else
					{
						csp::Time t;
						CurrentTime(&t);
						t = *timeout - t;
						sign32 millis = GetMilliSeconds(t);
						if (millis < 0 || WAIT_OBJECT_0 != SignalObjectAndWait(mutex,events[cond],static_cast<DWORD>(millis),FALSE))
						{
							return false;
						}
					}
				#else
					if (releaseWaitClaim(timeout,cond))
					{
						release();
					}
					else
					{
						return false;
					}
				#endif
				return true;
			}
			
			inline void signal(unsigned cond = 0,__CPPCSP_ALIGNED_USIGN32_PTR threadsRunningCount = NULL)
			{
				if (processIsWaiting[cond] != 0)
				{
					if (processIsWaiting[cond] == 1 && threadsRunningCount != NULL)
					{
						AtomicIncrement(threadsRunningCount);
					}
				
					processIsWaiting[cond] = 0;
					#ifdef CPPCSP_WINDOWS
						SetEvent(events[cond]);
					#else
						pthread_cond_signal(&(events[cond]));
					#endif
				}
			}
		};
		
		
		/**@internal
		*	Stores a process queue that supports "atomic" adds and removes
		*
		*	Not necessarily implemented using atomics - may use a mutex, but operations are guaranteed to be atomic
		*	with respect to each other.
		*
		*	This queue offers unit-time removal of a process from the head, and unit-time appending of
		*	single or chains of processes to the tail
		*/
		template <typename CONDITION>
		class _AtomicProcessQueue : public boost::noncopyable, private Primitive
		{
		private:
			Process* volatile head;
			Process* volatile tail;			
			
			CONDITION condition;
			
			static __CPPCSP_ALIGNED_USIGN32 ThreadsRunning;
			
			inline void claim() { condition.claim(); }
			inline void release() { condition.release(); }
		public:
			inline _AtomicProcessQueue()
				:	head(NULL),tail(NULL)
			{
				AtomicIncrement(&ThreadsRunning);
			}			
			
			inline ~_AtomicProcessQueue()
			{
				AtomicDecrement(&ThreadsRunning);
			}
						
			/**
			*	Pops the process at the head of the queue.
			*
			*	If the queue is empty, this call will block until there is a process to return,
			*	for the specified time.  If the time elapses without a process becoming
			*	available, NullProcessPtr is returned
			*
			*	@param time The pointer to the time to wait for.  If it is NULL, wait indefinitely
			*/
			inline ProcessPtr popHead(csp::Time* timeout = NULL);
			
			/**
			*	Pushes a single process onto the tail of the queue
			*/
			inline void pushProcess(ProcessPtr process);
			
			/**
			*	Pushes a pre-built chain of processes onto the tail of the queue
			*/
			inline void pushChain(ProcessPtr head,ProcessPtr tail);
			
			//TODO later remove this
			static volatile usign32 waitFP_calls;
		};				
	#ifdef CPPCSP_POSIX
		#ifndef CPPCSP_LINUX
			typedef _AtomicProcessQueue< Condition<1> > AtomicProcessQueue;
		#else
			#ifdef CPPCSP_ATOMICS
				typedef _AtomicProcessQueue< MutexAndEvent<PureSpinMutex> > AtomicProcessQueue;
			#else
				typedef _AtomicProcessQueue< MutexAndEvent<OSBlockingMutex> > AtomicProcessQueue;
			#endif		
		#endif
	#else
		#ifdef CPPCSP_ATOMICS
			typedef _AtomicProcessQueue< MutexAndEvent<PureSpinMutex> > AtomicProcessQueue;
		#else
			typedef _AtomicProcessQueue< MutexAndEvent<OSBlockingMutex> > AtomicProcessQueue;
		#endif
	#endif
		
#ifdef CPPCSP_ATOMICS
bool PureSpinMutex::tryClaim()
{
	return (NULL == _AtomicCompareAndSwap(&value,NULL,this));
}

void PureSpinMutex::release()
{
	AtomicPut(&value,static_cast<void*>(NULL));
}

bool SpinMutex::tryClaim(Kernel** pp)
{
	return (NULL == (*pp = AtomicCompareAndSwap(&value,static_cast<Kernel*>(NULL),GetKernel())));
}

void SpinMutex::release()
{
	AtomicPut(&value,static_cast<Kernel*>(NULL));
}

bool PureSpinMutex_TTS::tryClaim()
{
	if (NULL == AtomicGet(&value))			
		return (NULL == _AtomicCompareAndSwap(&value,NULL,this));
	else
		return false;
}

void PureSpinMutex_TTS::release()
{
	AtomicPut(&value,static_cast<void*>(NULL));
}

#endif //CPPCSP_ATOMICS

		template <typename CONDITION>
		ProcessPtr _AtomicProcessQueue<CONDITION>::popHead(csp::Time* timeout)
		{
			ProcessPtr retValue = NullProcessPtr;
			claim();
				while (head == NullProcessPtr)
				{
					//Need to wait for a process:					
					AtomicIncrement(&waitFP_calls);
					
					if (false == condition.releaseWaitClaim(timeout,0,&ThreadsRunning))
					{
						return NullProcessPtr;
					}										
				}
			
				retValue = head;
				head = retValue->nextProcess;
				if (retValue == tail)
				{
					tail = head;
				}				
				
			release();
			return retValue;
		}
		
		template <typename CONDITION>
		void _AtomicProcessQueue<CONDITION>::pushProcess(ProcessPtr ptr)
		{
			ptr->nextProcess = NullProcessPtr;
			claim();
				if (head == NullProcessPtr)
				{
					head = ptr;
					//Adding to an empty foreign queue, so signal:					
					condition.signal(0,&ThreadsRunning);
				}
				else
				{
					tail->nextProcess = ptr;
				}
				tail = ptr;
			release();
		}

		template <typename CONDITION>
		void _AtomicProcessQueue<CONDITION>::pushChain(ProcessPtr _head,ProcessPtr _tail)
		{
			_tail->nextProcess = NullProcessPtr;
			claim();
				if (head == NullProcessPtr)
				{
					//Adding to an empty foreign queue, so signal:
					condition.signal(0,&ThreadsRunning);
					head = _head;
				}
				else
				{
					tail->nextProcess = _head;
				}
				tail = _tail;
			release();
		}

		//End of group:
		/** @} */


				
	} //namespace internal
} //namespace csp
