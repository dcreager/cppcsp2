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

#include <boost/utility.hpp>

class AltTest;

namespace csp
{
	class CSProcess;
	class ThreadCSProcess;
	
	typedef CSProcess* CSProcessPtr;
	
	#define NullCSProcessPtr NULL

	//Windows defines GetCurrentThread and GetCurrentThreadId, so we avoid those names
	ThreadId CurrentThreadId();		
		
	namespace internal
	{
		class Kernel;
	
		Kernel* GetKernel();
	
	
		class TestInfo;
		
		class Process;

		typedef Process* ProcessPtr;
				
		typedef Process* AltingProcessPtr;
		/*
		class ProcessPtr
		{
			Process* ptr;
		public:
			explicit inline ProcessPtr(Process* _ptr) : ptr(_ptr) {}
			inline ProcessPtr() {}
		
			inline void operator=(Process* _ptr) {ptr = _ptr;}			
			inline operator Process*() const {return ptr;}
			inline Process* operator->() const {return ptr;}
		};

		//Well, potentially alting
		class AltingProcessPtr
		{
			Process* ptr;
		public:
			explicit inline AltingProcessPtr(Process* _ptr) : ptr(_ptr) {}
			inline AltingProcessPtr() {}
		
			inline void operator=(Process* _ptr) {ptr = _ptr;}			
			
			inline operator Process*() const {return ptr;}
			inline Process* operator->() const {return ptr;}			
		};
		
		inline bool operator==(ProcessPtr p,AltingProcessPtr q)
		{
			return (Process*)p == (Process*)q;
		}
		inline bool operator==(AltingProcessPtr p,ProcessPtr q)
		{
			return (Process*)p == (Process*)q;
		}
		*/		
		
		typedef ProcessPtr TimeoutId;
		
		#define NullProcessPtr (static_cast<csp::internal::ProcessPtr>(NULL))
		
		class Kernel;
		class TimeoutQueue;
		class Primitive;
		template <typename CONDITION>
		class _AtomicProcessQueue;
		
#ifdef CPPCSP_LONGJMP
		typedef jmp_buf Context;
#endif
#ifdef CPPCSP_FIBERS
		typedef void* Context;
#endif
				
		class Process : public boost::noncopyable
		{
		private:			
			inline static ProcessPtr CreateInitialProcess();			
			
		/*
		protected:
			void startInCurThread();
			void startInNewThread();
		*/
		private:
			ProcessPtr nextProcess;								
						
			//0 means not alting
			__CPPCSP_ALIGNED_USIGN32 alting;
			
			Time timeout;
			///Only used when in the timeout queue:
			ProcessPtr timeout_nextProcess;
			ProcessPtr* timeout_prevProcessPtr;

		protected:
			Kernel* kernel;			
			ThreadId threadId;			
			Context context;			
			unsigned char* stackPointer;			
			usign32 stackSize;
		public:			
			virtual void runProcess() = 0;
			virtual void endProcess() = 0;
			
			inline Process(Kernel* _kernel,ThreadId _threadId,usign32 _stackSize)
				:	nextProcess(NULL),					
					alting(0),
					timeout_nextProcess(NULL),
					timeout_prevProcessPtr(NULL),
					kernel(_kernel),
					threadId(_threadId),
					stackSize(_stackSize)
			{
			}
			
			inline virtual ~Process() {}

			friend class Kernel;
			friend class TimeoutQueue;
			template <typename CONDITION>					
			friend class _AtomicProcessQueue;
			friend class Primitive;
			friend class ::AltTest;
			friend void csp::Start_CPPCSP();						
		};	
		
		class __InitialProcess : public Process
			{
			public:
			virtual void runProcess() {};
			virtual void endProcess() {};
			inline __InitialProcess(Kernel* _kernel,ThreadId _threadId)
				:	Process(_kernel,_threadId,0)
				{
				}
			};
			
	
			ProcessPtr Process::CreateInitialProcess()
			{
				ProcessPtr process(new __InitialProcess(GetKernel(),CurrentThreadId()) );
				return process;
			}	
		
		class Primitive
		{
		protected:
			static Process* currentProcess();
			
			static ThreadId currentThread();
			
			static ThreadId getThreadId(Process*);
			
			///Frees an entire process chain.  They must belong to the same thread, and must not be involved in an alt
			static void freeProcessChain(ProcessPtr head,ProcessPtr tail);						
			
			///Always frees the process.  Do not use on processes that may be alting
			static void freeProcessNoAlt(ProcessPtr);
			
			//Schedules another process but does not put this one back on the run queue
			static void reschedule();
			
			//Schedules another process and puts this one back on the run queue
			static void yield();
			
			//Spins.  On a 1-CPU system, this will yield (no point spinning when we are using the only CPU!), otherwise it will return directly
			//If the count gets above a certain number, a yield may take place anyway
			static void spin(int count);
			
			static TimeoutId addTimeoutAlt(csp::Time* timeout,AltingProcessPtr proc); // {return GetKernel()->data.timeoutQueue.addTimeoutAlt(proc,time);}
			
			static bool removeTimeout(TimeoutId id); // {return GetKernel()->data.timeoutQueue.removeTimeout(id);}
			
			static inline ProcessPtr getNextProcess(ProcessPtr proc) {return proc->nextProcess;}
			
			#define ___ALTING_NOT 0
			#define ___ALTING_ENABLE 1
			#define ___ALTING_GUARDSREADY 2
			#define ___ALTING_WAITING 3
			
			static inline void altEnabling(AltingProcessPtr proc)
			{
				//No-one else should have done anything, so just put ourselves to enable:
				AtomicPut(&(proc->alting),___ALTING_ENABLE);
			}
			///True if it should wait, false if guards are ready
			static inline bool altShouldWait(AltingProcessPtr proc)
			{
				//Try to change from enabling to waiting
				//If we cannot do it, the state must have been changed to guards-ready by a guard,
				//in which case the alter should not wait, but instead proceed to the disabling sequence
				return ___ALTING_ENABLE == AtomicCompareAndSwap(&(proc->alting),___ALTING_ENABLE,___ALTING_WAITING);
			}
			static void altFinish(AltingProcessPtr proc)
			{
				//Just set us to not-alting:
				AtomicPut(&(proc->alting),___ALTING_NOT);
			}
			
			///Frees the process, taking account of its alting status
			static void freeProcessMaybe(AltingProcessPtr proc)
			{
				if (proc != NullProcessPtr)
				{
			
					usign32 state = AtomicCompareAndSwap(&(proc->alting),___ALTING_ENABLE,___ALTING_GUARDSREADY);
			
					//if (___ALTING_ENABLE == state)
						//They were enabling.  We changed the state.  No need to wake them up.  
						//Do nothing, and return
					//if (___ALTING_GUARDSREADY == state)
						//They have already been alerted that one or more guards are aready.  No need to wake them up.
						//Do nothing, and return
					if (___ALTING_NOT == state)				
					{
						//Not alting - free as normal:
						freeProcessNoAlt(proc);
					}
					else if (___ALTING_WAITING == state)
					{
						//They were waiting.  Try to atomically cmp-swap the state to guards-ready, so
						//that we are the one who wakes them up
						if (___ALTING_WAITING == AtomicCompareAndSwap(&(proc->alting),___ALTING_WAITING,___ALTING_GUARDSREADY))
						{
							//We made the change, so we should wake them up
							freeProcessNoAlt(proc);
						}
					
						//Otherwise, someone else must have changed the state from waiting to guards-ready
						//Therefore we don't need to wake them up after all
						//Do nothing, and return
					}				
				}
			}
			
		inline static void addProcessToQueue(ProcessPtr* head,ProcessPtr* tail,ProcessPtr process)
		{
			if (process != NullProcessPtr)
			{
				process->nextProcess = NullProcessPtr;
				if (*head == NullProcessPtr)
				{
					*head = *tail = process;
				}
				else
				{
					(*tail)->nextProcess = process;
					*tail = process;
				}
			}
		}
		
		inline static void addProcessToQueueAtHead(ProcessPtr* head,ProcessPtr* tail,ProcessPtr process)
		{
			if (process != NullProcessPtr)
			{
				if (*head == NullProcessPtr)
				{
					process->nextProcess = NullProcessPtr;
					*head = *tail = process;
				}
				else
				{
					process->nextProcess = *head;
					*head = process;
				}
			}
		}		


		inline static void addProcessToQueueAtHead(ProcessPtr volatile * head,ProcessPtr volatile * tail,ProcessPtr process)
		{
			if (process != NullProcessPtr)
			{
				if (*head == NullProcessPtr)
				{
					process->nextProcess = NullProcessPtr;
					*head = *tail = process;
				}
				else
				{
					process->nextProcess = *head;
					*head = process;
				}
			}
		}		

		inline static void addProcessChainToQueue(ProcessPtr* head,ProcessPtr* tail,ProcessPtr processHead,ProcessPtr processTail)
		{
			if (processHead != NULL)
			{
				processTail->nextProcess = NULL;
				if (*head == NullProcessPtr)
				{
					*head = processHead;
					*tail = processTail;
				}
				else
				{
					(*tail)->nextProcess = processHead;
					*tail = processTail;
				}
			}
		}	
		};
		
	} //namespace internal
		
}//namespace csp
