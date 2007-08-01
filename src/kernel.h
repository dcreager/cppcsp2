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

#ifndef INCLUDED_FROM_CPPCSP_H
#error This file should only be included by csp.h, not individually
#endif

#include <vector>
#include <list>
#include <algorithm>

#include <iostream>

namespace csp
{

	ThreadId CurrentThreadId();

	namespace internal
	{
	
	#ifdef CPPCSP_LONGJMP
		typedef unsigned char* ProcessDelInfo;
	#endif
	#ifdef CPPCSP_FIBERS
		typedef void* ProcessDelInfo;
	#endif
	
		template <typename T>
		class OneUsePointer
		{
		private:
			T* pointer;
			inline void operator=(const OneUsePointer<T>&) {};
		public:
			inline T* operator -> () {return pointer;};
			inline OneUsePointer(T* _pointer) : pointer(_pointer) {};
		};

		class Kernel;

		Kernel* GetKernel();

		class TimeoutQueue : public boost::noncopyable, private Primitive
		{
		private:
			ProcessPtr headNoAlt;
			ProcessPtr headAlt;
			
			inline static TimeoutId _addTimeout(ProcessPtr* prev,ProcessPtr process,const Time* timeout)
			{
				process->timeout = *timeout;
				while (*prev != NULL)
				{
					/*
					* It makes little difference whether we use less-than, or less-than-or-equal here
					* Its end effect is the order that the processes with exactly the same timeout 
					* will be put back on the run queue
					* less-than gives FIFO, less-than-or-equal gives LIFO.
					* While FIFO feels better, LIFO would be quicker for lots of identical timeouts
					* (add each to head of identical, not tail)
					* and we offer no guarantee about run-queue ordering anyway
					*/
					if (*timeout <= (*prev)->timeout)
					{
						process->timeout_nextProcess = *prev;
						process->timeout_prevProcessPtr = prev;
						(*prev)->timeout_prevProcessPtr = &(process->timeout_nextProcess);
						*prev = process;
						return process;
					}
					
					prev = &((*prev)->timeout_nextProcess);
				}
				
				//If we get here, we've reached the end of the timeout queue:
				process->timeout_nextProcess = NULL;
				process->timeout_prevProcessPtr = prev;
				*prev = process;
				return process;
			}
			
			///proc must not be NULL
			///Blanks timeout_prevProcessPtr as it goes along
			inline void _findTimeouts(ProcessPtr proc,ProcessPtr* timedoutHead,ProcessPtr* timedoutTail,bool buildNormalChain)
			{
				Time cur;
				CurrentTime(&cur);
					
				if (cur >= proc->timeout)
				{
					*timedoutHead = proc;
					*timedoutTail = proc;
					
					proc->timeout_prevProcessPtr = NULL;					
					if (buildNormalChain)
					{
						proc->nextProcess = proc->timeout_nextProcess;
					}
					proc = proc->timeout_nextProcess;
						
					//Keep going until we find a timeout that has not expired:
					while (proc != NULL && cur >= proc->timeout)
					{
						*timedoutTail = proc;
						proc->timeout_prevProcessPtr = NULL;
						if (buildNormalChain)
						{
							proc->nextProcess = proc->timeout_nextProcess;
						}
						proc = proc->timeout_nextProcess;
					}					
				}
			}
		public:
			inline TimeoutQueue()
				:	headNoAlt(NULL),headAlt(NULL)
			{				
			}
			
			
			///Adds the absolute timeout, even if it has already elapsed.  Linear time.
			inline TimeoutId addTimeoutNoAlt(ProcessPtr process, const Time* timeout)
			{				
				return _addTimeout(&headNoAlt,process,timeout);				
			}
			
			///Adds the absolute timeout, even if it has already elapsed.   Linear time.
			inline TimeoutId addTimeoutAlt(ProcessPtr process, const Time* timeout)
			{				
				return _addTimeout(&headAlt,process,timeout);				
			}
			
			inline bool haveTimeouts() const
			{
				return (headNoAlt != NULL) || (headAlt != NULL);
			}
			
			///Unit time, only call if haveTimeouts() returns true
			inline Time soonestTimeout() const
			{				
				if (headNoAlt != NULL)
				{
					if (headAlt != NULL)
					{
						return std::min(headNoAlt->timeout,headAlt->timeout);
					}
					else
					{
						return headNoAlt->timeout;
					}
				}
				else if (headAlt != NULL)
				{
					return headAlt->timeout;					
				}
				else
				{
					//We shouldn't ever reach here, but just in case:
					return Seconds(0);
				}
			}
			
			///Linear time
			inline void checkTimeouts()
			{
				if (headNoAlt != NULL)
				{
					ProcessPtr timedoutHead = NULL;
					ProcessPtr timedoutTail = NULL;
					
					_findTimeouts(headNoAlt,&timedoutHead,&timedoutTail,true);
					
					//Now add all the expired timeouts back onto the run queue in one go:
					
					if (timedoutHead != NULL)
					{
						headNoAlt = timedoutTail->timeout_nextProcess;
						freeProcessChain(timedoutHead,timedoutTail);
					}
				}
				
				if (headAlt != NULL)
				{
					ProcessPtr timedoutHead = NULL;
					ProcessPtr timedoutTail = NULL;
					
					_findTimeouts(headAlt,&timedoutHead,&timedoutTail,false);
					
					if (timedoutHead != NULL)
					{
						headAlt = timedoutTail->timeout_nextProcess;
					
						//Now step through all the expired timeouts :
					
						for (ProcessPtr proc = timedoutHead;proc != timedoutTail;)
						{
							ProcessPtr proc2 = proc->timeout_nextProcess;
														
							freeProcessMaybe(proc);
							
							proc = proc2;
						}
					}
				}				
			}
			
			/**
			*	Returns true if the timeout was still in the queue
			*	False if the timeout was not in the queue
			*
			*	NOTE: true does not necessarily mean that the timeout had not expired.  It simply means
			*	that the timeout had not yet been removed from the queue
			*	False means that the timeout was removed from the queue - either because
			*	it had expired, or because a previous removeTimeout call had removed it
			*
			*	This is now a unit-time operation.
			*/
			inline bool removeTimeout(TimeoutId timeoutId)
			{
				if (timeoutId->timeout_prevProcessPtr != NULL)
				{
					*(timeoutId->timeout_prevProcessPtr) = timeoutId->timeout_nextProcess;
					if (timeoutId->timeout_nextProcess != NULL)
						timeoutId->timeout_nextProcess->timeout_prevProcessPtr = timeoutId->timeout_prevProcessPtr;
					timeoutId->timeout_nextProcess = NULL;
					timeoutId->timeout_prevProcessPtr = NULL;										
					
					return true;
				}
				else
				{
					return false;
				}
			}
		};
		
		template < typename FUNCTIONPOINTER >
		class FunctionStack
		{
		public:
			std::vector<FUNCTIONPOINTER> stack;

			typedef FUNCTIONPOINTER Function;			
			
			inline void pushFunction(Function f)
			{
				stack.push_back(f);
			}
			
			inline void removeFunction(Function f)
			{
				stack.erase(
					std::remove_if(
						stack.begin(),
						stack.end(),
						std::bind2nd(std::equal_to< Function >(), f)
					),
					stack.end()
				);
				//Simple, really... :(					
			}			
		};
		
	class Kernel : public boost::noncopyable
	{
	private:
		//Hide implementation from our friends:
		class KernelData : public boost::noncopyable
		{
		private:
			static KernelData* originalThreadKernelData;
			static bool deadlocked;
		
			internal::ProcessPtr initialProcess;
			internal::ProcessPtr currentProcess;		
	
			AtomicProcessQueue runQueue;
			TimeoutQueue timeoutQueue;
			
			ThreadId threadId;

			//a rolling list of the previous blocks, ready for deadlocks		
			static std::list<internal::Process const *> blocks; 
			static PureSpinMutex blocksMutex;
			inline static bool RecordBlock(KernelData* data)
			{
				blocksMutex.claim();
					blocks.push_back(data->currentProcess);
					if (blocks.size() > 32)
						blocks.pop_front();
				blocksMutex.release();
				return true;
			}
			inline static void DumpBlocks(std::ostream& out)
			{
				blocksMutex.claim();
					out << "Block list (oldest first):" << std::endl;
					int i = 0;
					for (std::list<internal::Process const *>::const_iterator it = blocks.begin();it != blocks.end();it++)
					{
						out << i << ": " << *it << std::endl;
					}
				blocksMutex.release();
			}
	
			FunctionStack< bool (*)(KernelData*) > scheduleFunctions;
			FunctionStack< bool (*)(KernelData*,internal::ProcessPtr,internal::ProcessPtr) > addProcessFunctions;
			
			FunctionStack< void (*)(Kernel*) > initThreadFunctions;
			FunctionStack< void (*)(Kernel*) > destroyThreadFunctions;
			
			std::list< std::pair<ProcessPtr,ProcessDelInfo> > stacksToDelete;
			
			friend class Kernel;			
			friend class TestInfo;
			friend void csp::Start_CPPCSP();			
			
			inline KernelData()
				:	initialProcess(NullProcessPtr),
					currentProcess(NullProcessPtr)								
			{
			}
			
			void init(ThreadId _threadId)
			{
				threadId = _threadId;
			}
					
		} data;
		
	inline TimeoutQueue* getTimeoutQueue()
	{
		return &(data.timeoutQueue);
	}
		
	inline void pushScheduleFunction(bool (*fn)(KernelData*) ) {data.scheduleFunctions.pushFunction(fn);}
	inline void removeScheduleFunction(bool (*fn)(KernelData*) ) {data.scheduleFunctions.removeFunction(fn);}
	
	inline void pushAddProcessFunction(bool (*fn)(KernelData*,internal::ProcessPtr,internal::ProcessPtr) ) {data.addProcessFunctions.pushFunction(fn);}
	inline void removeAddProcessFunction(bool (*fn)(KernelData*,internal::ProcessPtr,internal::ProcessPtr) ) {data.addProcessFunctions.removeFunction(fn);}
	
	inline void pushInitThreadFunction(void (*fn)(Kernel*) ) {data.initThreadFunctions.pushFunction(fn);}
	inline void removeInitThreadFunction(void (*fn)(Kernel*) ) {data.initThreadFunctions.removeFunction(fn);}	
	
	inline void pushDestroyThreadFunction(void (*fn)(Kernel*) ) {data.destroyThreadFunctions.pushFunction(fn);}
	inline void removeDestroyThreadFunction(void (*fn)(Kernel*) ) {data.destroyThreadFunctions.removeFunction(fn);}		
	
	inline internal::ProcessPtr currentProcess() { return data.currentProcess; }
	
	static bool ReSchedule(KernelData*);
	static bool AddProcess(KernelData*,internal::ProcessPtr,internal::ProcessPtr);
	//Initialises a new thread, to be used just after its creation/use in C++CSP	
	static void InitNewThread(Kernel* kernel);
	
	//Destroys all the resources allocated for a thread, to be called at the end of a thread's lifetime
	static void DestroyInThread(Kernel* kernel);



	static Kernel* AllocateThreadKernel();
	static void DestroyThreadKernel();

	
	inline void reschedule(ProcessPtr curProcess)
	{
		data.currentProcess = curProcess;
		reschedule();
	}
	
	inline void reschedule()
	{		
		//Originally, this code used the top (commented version).
		//However, this loop will not always finish - when a process places its final call to reschedule,
		//the call never completes
		//This never caused a problem on Linux with GCC, but under Windows with Visual Studio 2005,
		//this caused an access violation (it seems to keep track of the current iterators)
		//So instead I use the bottom version with simple array indexing.
	
		/*for (std::vector< bool (*)(KernelData*) >::reverse_iterator it = data.scheduleFunctions.stack.rbegin();it != data.scheduleFunctions.stack.rend();it++)
		{
			if (false == (*(*it)) (&data) )
			{
				return;
			}
		}*/
		
		for (int i = static_cast<int>(data.scheduleFunctions.stack.size()) - 1;i >= 0;i--)
		{
			if (false == (*(data.scheduleFunctions.stack[i])) (&data) )
			{
				return;
			}
		}
	}
	
	inline void addProcessChain(internal::ProcessPtr head,internal::ProcessPtr tail)
	{
		//There was a similar problem to the reschedule function in this function.
		//As best I can gather, a kernel can be destroyed while another thread is in the final
		//stages of adding a process chain (namely, it's done the last action but hasn't finished the loop)
		//So we use indices too
		/*
		for (std::vector< bool (*)(KernelData*,internal::ProcessPtr,internal::ProcessPtr) >::reverse_iterator it = data.addProcessFunctions.stack.rbegin();it != data.addProcessFunctions.stack.rend();it++)
		{
			if (false == (*(*it)) (&data,head,tail) )
			{
				return;
			}
		}
		*/
		
		for (int i = static_cast<int>(data.addProcessFunctions.stack.size()) - 1;i >= 0;i--)
		{
			if (false == (*(data.addProcessFunctions.stack[i])) (&data,head,tail) )
			{
				return;
			}
		}
	}


	inline void initNewThread()
	{
		//std::cerr << "This: " << this << std::endl;
		//std::cerr << "Size: " << data.initThreadFunctions.stack.size() << std::endl;
	
		//Must go forwards as our main initialisation must come first
		for (std::vector< void (*)(Kernel*) >::iterator it = data.initThreadFunctions.stack.begin();it !=  data.initThreadFunctions.stack.end();it++)
		{
			(*(*it)) (this);
		}
	}
	
	inline void destroyInThread()
	{
		//Must go backwards as our main initialisation must come last
		for (std::vector< void (*)(Kernel*) >::reverse_iterator it = data.destroyThreadFunctions.stack.rbegin();it !=  data.destroyThreadFunctions.stack.rend();it++)
		{
			(*(*it)) (this);
		}		
	}	
	
public:
	inline Kernel()				
	{
		data.init(this);
		pushScheduleFunction(&ReSchedule);
		pushScheduleFunction(&KernelData::RecordBlock);
		pushAddProcessFunction(&AddProcess);
		pushInitThreadFunction(&InitNewThread);
		pushDestroyThreadFunction(&DestroyInThread);
	}
	
	inline Kernel(Kernel* kernel,ProcessPtr initial)		
	{
		data.init(this);
		data.scheduleFunctions = kernel->data.scheduleFunctions;
		data.addProcessFunctions = kernel->data.addProcessFunctions;
		data.initThreadFunctions = kernel->data.initThreadFunctions;
		data.destroyThreadFunctions = kernel->data.destroyThreadFunctions;
		data.initialProcess = data.currentProcess = initial;
	}
	
	friend class csp::internal::Primitive;
	friend class csp::internal::TestInfo;
	
	friend class csp::internal::Process;
	friend class csp::ThreadCSProcess;
	friend class csp::CSProcess;
	
	friend void csp::CPPCSP_Yield();	
	friend void csp::SleepFor(const Time& time);
	friend void csp::SleepUntil(const Time& time);


	friend Kernel* csp::internal::GetKernel();
	friend ThreadId csp::CurrentThreadId();

	friend void csp::Start_CPPCSP();

	friend void csp::End_CPPCSP();
	
	static void ThreadFunc(void* value);

	#ifdef CPPCSP_LONGJMP	
		//This is my ugly function:
		static void SetJmp_RunProcess(csp::internal::ProcessPtr gproc,jmp_buf* gjumpBackTo);
		static void SetJmp_Start(csp::internal::ProcessPtr pass_proc,jmp_buf* pass_jumpBackTo);
	#endif
	
	#ifdef CPPCSP_FIBERS
		static void CALLBACK Fiber_Start(void* param);
	#endif
};

		class TestInfo
		{
		protected:
			typedef Kernel::KernelData KernelData;
		
			static usign32 processesOnRunQueue();

			inline static void addScheduleFunction(bool (*fn)(KernelData*) )
			{
				GetKernel()->pushScheduleFunction(fn);
			}
			
			inline static void removeScheduleFunction(bool (*fn)(KernelData*))
			{
				GetKernel()->removeScheduleFunction(fn);
			}
	
			inline static void addAddProcessFunction(bool (*fn)(KernelData*,internal::ProcessPtr,internal::ProcessPtr) )
			{
				GetKernel()->pushAddProcessFunction(fn);
			}
			
			inline static void removeAddProcessFunction(bool (*fn)(KernelData*,internal::ProcessPtr,internal::ProcessPtr))
			{
				GetKernel()->removeAddProcessFunction(fn);
			}			

			//Looks pointless, but needed because of friends and such			
			static internal::ProcessPtr getProcessPtr(CSProcess* process);
			
			inline static internal::ProcessPtr currentProcess()
			{
				return GetKernel()->currentProcess();
			}
			
			inline static ThreadId getThreadId(KernelData* data)
			{
				return data->threadId;
			}
		};
		
		
	/*	
	inline CSProcess* GetCurrentProcess()
	{
		return GetKernel()->currentProcess();
	}
	*/

} //namespace internal
			
 
} //namespace csp

