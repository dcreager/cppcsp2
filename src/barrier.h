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

/** @file src/barrier.h
*	@brief Contains the header for the Barrier class
*/

#ifndef INCLUDED_FROM_CPPCSP_H
#error This file should only be included by csp.h, not individually
#endif

#include <set>
#include <map>

namespace csp
{	
	namespace internal
	{
		/**@internal
		*
		*	Used for implementation of cross-thread barriers.
		*
		*	The mutex only needs to be claimed for two operations - the final sync operation (which may be caused by a resign), and the enroll operations.
		*
		*	Each sync operation can be done non-atomically, unless it is the last in its thread - in which case it performs an atomic subtraction.
		*	The final sync of all the threads claims the mutex.
		*/
		template <typename MUTEX>
		class _InterThreadBarrier : private internal::Primitive, public boost::noncopyable
		{
			class Data
			{
			public:
				volatile usign32 leftToSync;
				volatile usign32 enrolled;
				volatile ProcessPtr queueHead;
				volatile ProcessPtr queueTail;
			};
			std::map<ThreadId,Data> processes;
			__CPPCSP_ALIGNED_USIGN32 threadsLeftToSync;
			MUTEX mutex;			
			
			/**
			*	@return true if the sync completed, false if it didn't (i.e. false means we are blocked)
			*
			*	May block temporarily to grab the mutex
			*/			
			inline bool syncWholeThread(const ProcessPtr& process,usign32* numWhoSynced)
			{
				//std::cerr << "syncWholeThread " << this << std::endl;
			
					typename MUTEX::End mutexEnd(mutex.end());

					usign32 val = AtomicDecrement(&threadsLeftToSync);
					
					if (val == 0)
					{
						//We can complete the sync, as long as nobody enrolls in the mean-time.  Must grab the mutex:
						mutexEnd.claim();
					
						//All the other processes were blocked, so the only person that could have come along since
						//our atomic decrement was an enrolling process.  If they've enrolled (but not synced)
						//we must also wait - they will complete the sync
					
						if (AtomicGet(&threadsLeftToSync) == 0)
						{
							//We need to put the count back, ready for any processes that we free who sync quickly.
							//First we must make a pass to count the number that will be left:
							
							usign32 threadsLeft = 0;
							for (typename std::map<ThreadId,Data>::iterator it = processes.begin();it != processes.end();it++)
							{
								if (it->second.enrolled > 0)
								{
									threadsLeft += 1;
								}
							}
							
							AtomicPut(&threadsLeftToSync,threadsLeft);
							
							if (numWhoSynced != NULL)
								*numWhoSynced = 0;
						
							//No-one enrolled before we claimed the mutex - sync by freeing everyone in the map (except us!)
							for (typename std::map<ThreadId,Data>::iterator it = processes.begin();it != processes.end();)
							{
								ProcessPtr queueHead = it->second.queueHead;
								ProcessPtr queueTail = it->second.queueTail;
								
								if (numWhoSynced != NULL)
									*numWhoSynced += it->second.enrolled;
							
								if (it->second.enrolled > 0)
								{
									it->second.queueHead = NullProcessPtr;
									it->second.queueTail = NullProcessPtr;
									it->second.leftToSync = it->second.enrolled;
									
									it++;
								}
								else
								{									
								#ifdef CPPCSP_MSVC
									it = processes.erase(it);
								#else
									processes.erase(it++);
								#endif
								}							
							
								if (queueHead != NullProcessPtr)
								{
									if (queueHead == process)
									{
										//Miss out ourselves:
										ProcessPtr next = getNextProcess(process);
										if (next != NULL)
										{
											freeProcessChain(next,queueTail);
										}										
									}
									else
									{
										//We weren't in there - free them all:
										freeProcessChain(queueHead,queueTail);
									}
								}								
							}
												
							mutexEnd.release();
							return true;
						}
						else
						{
							//Someone has enrolled.  We must sleep ourselves!
							mutexEnd.release();
							return false;
						}
					}
					else
					{
						//Not all threads are ready
						return false;
					}
			}						
			
		public:
			typedef Data* Key;

			inline _InterThreadBarrier()
				:	threadsLeftToSync(0)
			{
			}			

			//If it performs the sync, returns the number of processes who synced 
			//If it did not perform the sync itself, returns 0;
			inline usign32 sync(const Key key)
			{
				//temp__issueDebugReport("pre-sync ");
			
				const ProcessPtr process ( currentProcess() );
				addProcessToQueueAtHead(&(key->queueHead),&(key->queueTail),process);
				usign32 num = 0;
				bool synced;
				if (--(key->leftToSync) == 0)
				{
					synced = syncWholeThread(process,&num);
				}
				else
				{
					//Not everyone in this thread is ready yet
					synced = false;
				}

				//temp__issueDebugReport("post-sync");
				
				if (false == synced)
				{
					//We may already be back on the run queue by now, but that doesn't matter
					reschedule();
				}
				
				return num;
			}
		
			inline Key enroll()
			{
				const ThreadId& threadId = currentThread();
				typename MUTEX::End mutexEnd = mutex.end();
				mutexEnd.claim();
				
				//It's ok if we add ourselves to the map, as no-one else will be altering the map while we hold the mutex
				
				typename std::map<ThreadId,Data>::iterator it = processes.find(threadId);
				
				if (it == processes.end())
				{
					Data data;
					data.leftToSync = 1;
					data.enrolled = 1;
					data.queueHead = NullProcessPtr;
					data.queueTail = NullProcessPtr;
					it = processes.insert(std::make_pair(threadId,data)).first;
					AtomicIncrement(&threadsLeftToSync);
				}
				else
				{
					it->second.leftToSync += 1;
					it->second.enrolled += 1;
					if (it->second.leftToSync == 1)
					{
						//Everyone else was done already - but not any more!
						AtomicIncrement(&threadsLeftToSync);
					}
				}
				
				mutexEnd.release();
				
				return &(it->second);
			}
			
			inline void halfEnroll()
			{
				typename MUTEX::End mutexEnd = mutex.end();
				mutexEnd.claim();
				
				AtomicIncrement(&threadsLeftToSync);
				
				mutexEnd.release();
			}
			
			inline Key completeEnroll()
			{
				//temp__issueDebugReport("pre-completeEnroll");

				const ThreadId& threadId = currentThread();
				typename MUTEX::End mutexEnd = mutex.end();
				mutexEnd.claim();
				
				//It's ok if we add ourselves to the map, as no-one else will be altering the map while we hold the mutex
				
				typename std::map<ThreadId,Data>::iterator it = processes.find(threadId);
				
				if (it == processes.end())
				{
					Data data;
					data.leftToSync = 1;
					data.enrolled = 1;
					data.queueHead = NullProcessPtr;
					data.queueTail = NullProcessPtr;
					it = processes.insert(std::make_pair(threadId,data)).first;
					
					//threadsLeftToSync is already one greater than it should be because of our half-enrollment
					//so don't touch it
				}
				else
				{
					it->second.leftToSync += 1;
					it->second.enrolled += 1;
					//threadsLeftToSync is already one greater than it should be because of our half-enrollment
					//so we should reverse that, if appropriate:
					if (it->second.leftToSync == 1)
					{
						//Everyone else was done already - but not any more!
						//no need to reverse
					}
					else
					{
						//reverse it:
						AtomicDecrement(&threadsLeftToSync);
					}					
				}
				
				mutexEnd.release();
				
				//temp__issueDebugReport("post-completeEnroll");
				
				
				return &(it->second);				
			}
			
			//Returns true if the barrier is now empty
			inline bool resign(const Key key)
			{
				//std::cerr << "resign " << this << std::endl;
				//temp__issueDebugReport("pre-resign");				

				key->enrolled -= 1;
				if (--(key->leftToSync) == 0)
				{
					usign32 num = 1; //Just so long as it's not zero!
					syncWholeThread(NullProcessPtr,&num);
					
					return (num == 0);
				}
					
				return false;
			}			
			
			inline ~_InterThreadBarrier()
			{
				if (false == std::uncaught_exception())
				{
					//Claim the mutex to make sure no-one is currently finishing a previous sync:
					typename MUTEX::End mutexEnd = mutex.end();
					mutexEnd.claim();
					
					//if there are classes synced on the barrier when it is destroyed, we should do something about it
					if (processes.size() != 0 || AtomicGet(&threadsLeftToSync) > 0)
					{
						throw BarrierError("InterThreadBarrier was destroyed while some processes were still enrolled on it");
					}
					
					//No point in releasing the mutex - it's about to be destroyed
					//But may as well do it to be tidy:
					mutexEnd.release();
				}
			}
		};
		
		typedef _InterThreadBarrier<PureSpinMutex> InterThreadBarrier;
	
	}
	
	
	
	class BarrierEnd;
	class AltBarrierEnd;
	class ScopedBarrierEnd;
	class ScopedAltBarrierEnd;


	namespace internal
	{
		//TODO later document this class internally
		class BarrierBase //: public ReferenceCount
		{
		protected:
			/*
			inline BarrierBase(bool deleteWhenReferenceCountHitsZero = false)
				:	ReferenceCount(deleteWhenReferenceCountHitsZero)
			{
			}
			*/
			
			inline virtual ~BarrierBase()
			{
			}
			
			/**
			*	@return The key for the enrollment.  <b>Must not be NULL</b> (use a dummy value if necessary)
			*	TODO later mention the significance (cannot be this object)
			*/
			virtual void* enroll() = 0;
			
			virtual void halfEnroll() = 0;
			
			virtual void* completeEnroll() = 0;
			
			virtual void resign(void* key) = 0;
			
			virtual void sync(void* key) = 0;
		
			typedef BarrierBase* Pointer;
			
			friend class csp::BarrierEnd;			
		};
		
		//TODO later document this class internally
		class AltBarrierBase : public BarrierBase
		{
		protected:
			/*
			inline AltBarrierBase(bool deleteWhenReferenceCountHitsZero = false)
				:	BarrierBase(deleteWhenReferenceCountHitsZero)
			{
			}
			*/
		
			virtual Guard* guard(void* key) = 0;
			
			typedef AltBarrierBase* Pointer;
			
			friend class csp::AltBarrierEnd;			
		};
		
		
	} //namespace internal	
	
	/**
	*	The "end" of a barrier for use.
	*
	*	Barriers cannot be used directly, instead each process enrolled on the barrier must possess
	*	an end of the barrier.
	*
	*	Barrier ends are usually given out in the form Mobile<BarrierEnd>.  Mobile is used to ensure
	*	that barrier-ends are not unintentionally duplicated, which would lead to confusion.  While it
	*	may not be immediately apparent, using BarrierEnd without Mobile would cause a lot of trouble.
	*	See the Mobile class for more details on its use and semantics.  If you do wish to copy a BarrierEnd
	*	to pass to a sub-process, you can use the makeNonEnrolledCopy() or makeEnrolledCopy() functions accordingly.
	*
	*	A barrier end can be in one of two states - enrolled or non-enrolled.  Syncing is only possible
	*	when the end is enrolled.  Multiple enroll() calls will not cause an error, and neither will multiple
	*	resign() calls, provided they are made from the same process that will make/has made the sync() calls (see below).  
	*	A barrier end instance will always be associated with the same barrier.
	*
	*	Once an end is enrolled on the barrier, its sync() calls must all be from the same process.  Calling
	*	sync() on the same barrier end from two different processes (concurrently or consecutively) will cause
	*	undefined behaviour.  If you wish to transfer a barrier end from one process to another, you must
	*	first resign() in the original process and then call enroll() in the new process.  Alternatively,
	*	the better method is probably to create a new end for the new process (either from the original
	*	Barrier or using makeEnrolledCopy() ) and resign from the original end.
	*
	*	@see Barrier
	*	@see ScopedBarrierEnd
	*/
	class BarrierEnd : public boost::noncopyable
	{
	private:
		internal::BarrierBase::Pointer barrier;
		void* key;
		
		/**
		*	The default constructor
		*/
		inline BarrierEnd()
			:	barrier(NULL),key(NULL)
		{
		}
	public:
		/**@internal
		*	Constructor for internal use only
		*/	
		inline explicit BarrierEnd(internal::BarrierBase* _barrier,void* _key = NULL)
			:	barrier(_barrier),key(_key)
		{
		}		
		
		/**
		*	The destructor.  Throws a BarrierError if the BarrierEnd is still enrolled, and 
		*	the destructor is not being called because of a previously thrown exception.
		*
		*	Destroying a BarrierEnd while it is still enrolled is an error.  The destructor resigns
		*	but throws a BarrierError afterwards (if safe to do so - it will not be thrown if the
		*	destructor is being called because of stack-unwinding).  If BarrierError is thrown, your
		*	application is in error and needs to be fixed.  Make sure you always resign from
		*	a BarrierEnd before it is destroyed.
		*
		*	If you wish this resignation to be done automatically without a BarrierError being thrown,
		*	please look at ScopedBarrierEnd instead.
		*
		*	@see ScopedBarrierEnd
		*/
		inline ~BarrierEnd()
		{
			if (key != NULL)
			{
				//They are destroying the end without resigning!
				//Tidy up for safety but issue an error:
				resign();
				
				if (false == std::uncaught_exception())
					throw BarrierError("A BarrierEnd (or AltBarrierEnd) was destroyed while still enrolled on a barrier - did you omit a resign() call?");
			}
		}
	
		/**
		*	Enrolls on the barrier.  This must be done by the process that wishes to enroll (i.e. the process that will then make the sync() and resign() calls).
		*
		*	Enroll may temporarily block.
		*
		*	If you want to construct a pre-enrolled barrier please look at the Barrier::enrolledEnd() and
		*	makeEnrolledCopy() functions.
		*
		*	Calling enroll() twice on the same BarrierEnd (without a resign() call inbetween) in the same process is guaranteed to have no effect (i.e. it will not enroll on the barrier twice).
		*/
		inline void enroll()
		{
			if (key == barrier)
			{
				key = barrier->completeEnroll();
			}
			else if (key == NULL)
			{
				key = barrier->enroll();
			}			
		}
		
		/**
		*	Resigns from the barrier.  This must be done by the same process that has previously called enroll() on the barrier.
		*
		*	Resign may temporarily block.
		*
		*	After resignation, you may call enroll() again to re-enroll on the same barrier.
		*
		*	Calling resign() on a BarrierEnd that the process is not currently enrolled on is guaranteed to have no effect.
		*/
		inline void resign()
		{
			if (key == barrier)
			{
				//We're only half-enrolled.  Need to fully enroll before resigning:
				key = barrier->completeEnroll();
			}
			
			if (key != NULL)
			{
				barrier->resign(key);
				key = NULL;
			}
		}
		
		/**
		*	Synchronises with the barrier.  This must be done by the same process that has previously called enroll() on the barrier (and has not yet called resign() since).
		*
		*	Sync will block until all the other processes on the barrier have also called sync().
		*
		*	It is an error to call sync() on a currently non-enrolled barrier end.  This will result
		*	in a BarrierError being thrown.
		*/
		inline void sync()
		{
			if (key == barrier)
			{
				//We're only half-enrolled.  Complete enrollment before syncing:
				key = barrier->completeEnroll();
			}
		
			if (key != NULL)
			{
				barrier->sync(key);
			}
			else
			{
				throw BarrierError("Attempt made to sync() on a barrier while not enrolled - did you not call enroll() first?");
			}
		}

		/**
		*	Makes a copy of this BarrierEnd (that uses the same Barrier) that is already enrolled on the barrier.		
		*/
		inline Mobile<BarrierEnd> makeEnrolledCopy()
		{
			barrier->halfEnroll();
			//Make sure an identical pointer is given for both arguments:
			//(base might not be equal to this, due to virtual function tables and such)			
			return Mobile<BarrierEnd>(new BarrierEnd(barrier,barrier));
		}
		
		/**
		*	Makes a copy of this BarrierEnd (that uses the same Barrier) that is not enrolled on the barrier.
		*/
		inline Mobile<BarrierEnd> makeNonEnrolledCopy()
		{
			return Mobile<BarrierEnd>(new BarrierEnd(barrier));
		}
		
		/**
		*	All BarrierEnd instances are inherently distinct.  However, they are deemed to be equal
		*	if they use the same underlying Barrier.  No consideration is given to whether either of
		*	them is currently enrolled.
		*/
		inline bool operator == (const BarrierEnd& end)
		{
			return barrier == end.barrier;
		}
		
		/**
		*	All BarrierEnd instances are inherently distinct.  This ordering is based on the underlying
		*	barrier.  No consideration is given to whether either of them is currently enrolled.
		*/
		inline bool operator < (const BarrierEnd& end)
		{
			return barrier < end.barrier;
		}

		friend class AltBarrierEnd;		
		
	};

	//TODO later add comparison operations	
	class AltBarrierEnd : public BarrierEnd
	{		
	private:
		internal::AltBarrierBase::Pointer altBarrier;
	
		/**
		*	Constructor for internal use only
		*/	
		inline explicit AltBarrierEnd(internal::AltBarrierBase::Pointer _barrier,void* _key = NULL)
			:	BarrierEnd(_barrier,_key),altBarrier(_barrier)
		{
		}
		
	public:

		/**
		*	The default constructor
		*/
		inline AltBarrierEnd()
			:	BarrierEnd()
		{
		}

		/*
		inline AltBarrierEnd(const AltBarrierEnd& end)
			:	BarrierEnd(end),altBarrier(end.altBarrier)
		{
		}
		*/
		
		inline ~AltBarrierEnd()
		{
			if (key != NULL)
			{
				//They are destroying the end without resigning!
				//Tidy up for safety but issue an error:
				resign();
				if (false == std::uncaught_exception())
					throw BarrierError("A BarrierEnd (or AltBarrierEnd) was destroyed while still enrolled on a barrier - did you omit a resign() call?");
			}
		}


		/**
		*	Assignment.
		*	The original end remains as it is, this new end (the one being assigned to) is assigned as non-enrolled,
		*	even if the end we are copying from was enrolled.
		*
		*	It is an error to assign to a barrier end that is currently enrolled.
		*/
		/*
		inline void operator=(const AltBarrierEnd& end)
		{
			if (key == NULL)
			{
				barrier = end.barrier;
				altBarrier = end.altBarrier;
			}
			else
			{
				throw BarrierError("An attempt was made to assign to an enrolled AltBarrierEnd.  Call resign() before overwriting a barrier end.");
			}
		}
		*/		
			
		/**
		*	Gets a guard for the barrier.  This must be used in an Alternative in a process that has previously called enroll() on the barrier (and has not yet called resign() since).
		*
		*	The Alternative will delete this guard so you don't have to.
		*
		*	Using a guard in a process not currently enrolled on the barrier has undefined behaviour.
		*
		*	@see Alternative
		*	@see Guard
		*	@return A guard representing the sync operation on this barrier
		*/		
		inline Guard* guard()
		{
			//TODO later make it throw a BarrierError in the guard if we are not enrolled
			return altBarrier->guard(key);
		}
		
		friend class AltBarrier;
	};
	
	//TODO later document
	Mobile<BarrierEnd> NoAlt(const Mobile<AltBarrierEnd>&);
	
	
	/**
	*	A scoped Barrier end.
	*
	*	ScopedBarrierEnd applies the scoped concept to the end of a barrier.
	*	It enrolls on the barrier when constructed, and when it is destroyed it resigns
	*	from the barrier, so that you do not have to remember to explicitly resign from the
	*	barrier.
	*
	*	It is used as follows:
	*	@code
		Barrier barrier;
		...
		{
			ScopedBarrierEnd end(barrier.end()); 
				//You can use barrier.end() or barrier.enrolledEnd() above
				//Either way, the barrier will be enrolled upon
			...
			end.sync();
			...
		} //Here, the ScopedBarrierEnd is destroyed, and the barrier is resigned from automatically
		@endcode
	*
	*	The automatic resignation from the barrier is useful for two reasons - firstly, it prevents
	*	you from forgetting to do so.  Secondly, the resignation automatically occurs even if an
	*	exception is thrown.  Otherwise you would have to declare a BarrierEnd outside of the try
	*	block and place a resignation in the catch block.
	*
	*	Some may find this automatic resignation to be too hidden, in the invisible destruction
	*	of the ScopedBarrierEnd.  If this is the case, look at using the more explicit BarrierEnd
	*	instead.
	*
	*	As its name suggests, ScopedBarrierEnd must only be declared as a local scope on-the-stack variable.  
	*	Allocating it on the heap loses all the benefits, as does allocating it as a class variable.
	*	
	*	See the @ref scoped "Scoped" page for more information on the scoped classes.
	*
	*	@ingroup scoped
	*/
	class ScopedBarrierEnd : public boost::noncopyable
	{		
		Mobile<BarrierEnd> end;		
	public:
		/**
		*	Constructs a ScopedBarrierEnd using the given BarrierEnd.
		*
		*	The supplied BarrierEnd can either be enrolled or not-enrolled.  Either way,
		*	the ScopedBarrierEnd constructor will ensure that it is enrolled on the barrier.
		*
		*	Due to Mobile semantics, the passed Mobile will be blanked.
		*
		*	@param _end A BarrierEnd of the Barrier to use
		*/
		inline explicit ScopedBarrierEnd(const Mobile<BarrierEnd>& _end)
			:	end(_end)
		{
			end->enroll();
		}
		
		
		inline explicit ScopedBarrierEnd(const Mobile<AltBarrierEnd>& _end)
			:	end(NoAlt(_end))
		{	
			end->enroll();		
		}		
		
		/**
		*	Destructor - automatically resigns the ScopedBarrierEnd from the Barrier.		
		*/
		inline ~ScopedBarrierEnd()
		{
			end->resign();
		}
	
		/**
		*	Syncs on the barrier.
		*
		*	Sync will block until all the processes enrolled on the barrier have also synced.
		*/
		void sync()
		{
			end->sync();
		}
			
	};

	//TODO later document this class.  And uncomment it!
	/*
	class ScopedAltBarrierMember : public boost::noncopyable
	{
		internal::AltBarrierBase::Pointer barrier;
		void* key;
	public:
		inline explicit ScopedAltBarrierMember(const AltBarrierEnd& end)
			:	barrier(end.altBarrier)
		{
			key = barrier->enroll();
		}
		
		inline ~ScopedAltBarrierMember()
		{
			barrier->resign(key);
		}
	
		void sync()
		{
			barrier->sync(key);
		}
		
		Guard* guard()
		{
			return barrier->guard(key);
		}
			
	};
	*/	


	/**
	*	A barrier that multiple processes can synchronize on.
	*
	*	A barrier is an object that allows multiple parallel processes to synchronize.  When one member
	*	of a barrier synchronizes, it will not complete that synchronization until all members
	*	of the barrier have synchronized.  @ref tut-barriers "Barriers" are described in the guide.
	*
	*	Barriers can be used for a wide variety of purposes, such as making sure a set of processes
	*	have all started up, or making sure they all finish a task before any of them continues, or
	*	to effect other sorts of synchronization between processes.  Unlike channels, barriers do not
	*	communicate any data, but a synchronization can involve an arbitrary number of processes,
	*	from one upwards.
	*
	*	Unlike most barrier mechanisms offered by operating systems, processes may enroll on (join) the 
	*	barrier at any time, and they may resign from (leave) the barrier at any time.  They can even enroll,
	*	then resign, then enroll again and so on, as many times as they like.  
	*
	*	Each synchronization cannot	complete until all current members of the barrier have also synchronized.
	*	Imagine two processes, A and B, are enrolled on the barrier.  A attempts to synchronize and is forced
	*	to wait (because B has not yet synchronized).  If C now enrolls on the barrier, before B synchronizes,
	*	A's synchronization will not complete until both B and C have synchronized.  C then synchronizes, 
	*	meaning that A and C are waiting for B to synchronize.  If B now resigns from the barrier, A and C
	*	will complete their synchronization because they are the only two remaining processes enrolled on the
	*	barrier and they are both waiting to synchronize.
	*
	*	All operations on a Barrier are done through BarrierEnd (or ScopedBarrierEnd).   Ends can
	*	be obtained using the end() and enrolledEnd() methods.
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	C++CSP v1.x had the Barrier class, but it was used directly, rather than with barrier-ends.  This allowed
	*	some flexibility (enroll() and resign() could be called from anywhere) but repeated enroll() or resign() calls by the same process
	*	could not be detected, a pointer or reference to the Barrier had to be given to processes using the barrier.
	*	The previous Barrier model also would have missed out of some performance optimisations that the new Barrier
	*	has internally.
	*
	*
	*	@see BarrierEnd
	*/
	class Barrier : private internal::BarrierBase, public boost::noncopyable
	{
	private:
		internal::InterThreadBarrier barrier;
		bool deleteSelfWhenBecomeEmpty;

		virtual void* enroll()
		{
			return barrier.enroll();
		}
		
		virtual void halfEnroll()
		{
			barrier.halfEnroll();
		}
		
		virtual void* completeEnroll()
		{
			return barrier.completeEnroll();
		}
		
		virtual void resign(void* key)
		{
			if (
				barrier.resign(static_cast<internal::InterThreadBarrier::Key>(key))
				&& deleteSelfWhenBecomeEmpty)
			{
				delete this;
			}
		}
		
		virtual void sync(void* key)
		{
			barrier.sync(static_cast<internal::InterThreadBarrier::Key>(key));
		}
		
		//TODO later document _deleteSelfWhenBecomeEmpty well
		inline Barrier(bool _deleteSelfWhenBecomeEmpty)
			:	deleteSelfWhenBecomeEmpty(_deleteSelfWhenBecomeEmpty)
		{			
		}	
	public:
		/**
		*	Default constructor.  Constructs an empty barrier.
		*/
		inline Barrier() : deleteSelfWhenBecomeEmpty(false) {}
		
		#ifdef CPPCSP_DOXYGEN
		//This behaviour is actually a result of the InterThreadBarrier's destruction,
		//but this is the sensible place to document it:
		
		/**
		*	Destroys the barrier.  If any processes are still enrolled on the barrier, a BarrierError
		*	will be thrown because your application is in error.		
		*/
		inline ~Barrier() {}
		#endif

		/**
		*	Gets a non-enrolled end of this barrier.
		*
		*	@see BarrierEnd
		*	@see ScopedBarrierEnd
		*	@see Mobile
		*/
		Mobile<BarrierEnd> end()
		{
			return Mobile<BarrierEnd>(new BarrierEnd(this));
		}
		
		
		/**
		*	Gets an already enrolled end of this barrier.
		*
		*	@see BarrierEnd
		*	@see ScopedBarrierEnd
		*	@see Mobile
		*/
		Mobile<BarrierEnd> enrolledEnd()
		{
			halfEnroll();
			//Make sure an identical pointer is given for both arguments:
			//(base might not be equal to this, due to virtual function tables and such)
			BarrierBase* base = this;
			return Mobile<BarrierEnd>(new BarrierEnd(base,base));
		}	
		
		friend class ScopedForking;
	};



} //namespace csp
