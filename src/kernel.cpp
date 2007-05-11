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

#include "cppcsp.h"
#include <iostream>
#include "thread_local.h"


using namespace csp;
using namespace csp::internal;
using namespace std;

std::list<internal::Process const *> Kernel::KernelData::blocks; 
PureSpinMutex Kernel::KernelData::blocksMutex;
Kernel::KernelData* Kernel::KernelData::originalThreadKernelData = NULL;
bool Kernel::KernelData::deadlocked = false;

template <>
__CPPCSP_ALIGNED_USIGN32 _AtomicProcessQueue< MutexAndEvent<OSBlockingMutex,1> >::ThreadsRunning = 0;
template <>
__CPPCSP_ALIGNED_USIGN32 _AtomicProcessQueue< MutexAndEvent<PureSpinMutex,1> >::ThreadsRunning = 0;
template <>
__CPPCSP_ALIGNED_USIGN32 _AtomicProcessQueue< Condition<1> >::ThreadsRunning = 0;

namespace
{
	DeclareThreadLocalPointer(Kernel) __gkernel;
}

void Kernel::ThreadFunc(void* value)
{
	Kernel* kernel = static_cast<Kernel*>(value);

	__gkernel = kernel;
	
	//std::cerr << "INITNEWTHREAD, kernel:" << __gkernel << std::endl;

	ProcessPtr mainProcess = kernel->currentProcess();

	kernel->initNewThread();

	//std::cerr << "DONE-INITNEWTHREAD" << std::endl;

	
	try
	{
		mainProcess->runProcess();
		//std::cerr << "PROCESS FINISHED RUNNING" << std::endl;
		mainProcess->endProcess();
		//std::cerr << "PROCESS ENDED" << std::endl;
		
	}
	catch (std::exception& e)
	{
		std::cerr << "Uncaught exception from process: " << e.what() << std::endl;
	}

	kernel->destroyInThread();	
	
	Kernel::DestroyThreadKernel();
	
	//std::cerr << "THREAD FINISHED" /*<< GetCurrentThread()*/ << std::endl;

}

template <>
volatile usign32 _AtomicProcessQueue< MutexAndEvent<OSBlockingMutex,1> >::waitFP_calls = 0;
template <>
volatile usign32 _AtomicProcessQueue< MutexAndEvent<PureSpinMutex,1> >::waitFP_calls = 0;
template <>
volatile usign32 _AtomicProcessQueue< Condition<1> >::waitFP_calls = 0;

namespace csp
{
	ThreadId CurrentThreadId()
	{
		return __gkernel;
	}

	namespace internal
	{
	
		internal::ProcessPtr TestInfo::getProcessPtr(CSProcess* process)
		{
			return static_cast<internal::ProcessPtr>(process);
		}
	
		Kernel* GetKernel()
		{
			return __gkernel;
		}	
		
		Kernel* Kernel::AllocateThreadKernel()
		{
			Kernel* kernel = new Kernel;
			__gkernel = kernel;
			return kernel;
		}
		
		void Kernel::DestroyThreadKernel()
		{
			delete __gkernel;
		}
		
		void Kernel::InitNewThread(Kernel* kernel)
		{
			kernel->data.threadId = csp::CurrentThreadId();
			#ifdef CPPCSP_FIBERS
				kernel->data.currentProcess->context = ConvertThreadToFiber(kernel->data.currentProcess);
			#endif
		}
		
		void Kernel::DestroyInThread(Kernel* kernel)
		{
			#ifdef CPPCSP_FIBERS
				ConvertFiberToThread();
			#endif
		
			//delete the initial process:
			delete kernel->data.initialProcess;
		}
		
		void ContextSwitch(Context* from, Context* to);
		
		bool Kernel::ReSchedule(KernelData* data)
		{					
			ProcessPtr oldProcess = data->currentProcess;

			Time t;
			Time* pt = NULL;
			
			try
			{
				do
				{
					pt = NULL;
					data->timeoutQueue.checkTimeouts();
			
					if (data->timeoutQueue.haveTimeouts())
					{
						t = data->timeoutQueue.soonestTimeout();
							pt = &t;
					}

					//Get the next process to run:
					data->currentProcess = data->runQueue.popHead(pt);
				}
				while (data->currentProcess == NullProcessPtr);
					
			}
			catch (DeadlockException)
			{
				std::cerr << "Deadlock!" << std::endl;
				
				KernelData::DumpBlocks(std::cerr);
			
				//switch to initial process and throw a DeadlockError:
				
				KernelData::deadlocked = true;
				AddProcess(KernelData::originalThreadKernelData,KernelData::originalThreadKernelData->initialProcess,KernelData::originalThreadKernelData->initialProcess);
				
				//We can't really do anything more, so block:
				data->runQueue.popHead(NULL);
			}
		
			
			//Switch to the new process:
			if (oldProcess != data->currentProcess)
			{
				ContextSwitch(oldProcess == NULL ? NULL : &(oldProcess->context),&(data->currentProcess->context));
				
				//oldProcess is always the process of this currently running function:
				
				if (data == KernelData::originalThreadKernelData && oldProcess == data->initialProcess)
				{
					//We are the very original process -- we might have been woken because everything has deadlocked
					if (KernelData::deadlocked)
					{
						//No need to claim the mutex -- we must be the only process running!
						throw DeadlockError(KernelData::blocks);
					}
				}
			}
			
			//Delete any stacks from old processes:
			for (list< pair<ProcessPtr, ProcessDelInfo> >::iterator it = data->stacksToDelete.begin();it != data->stacksToDelete.end();)
			{
				if (it->first == data->currentProcess)
				{
					it++;
				}
				else
				{
				#ifdef CPPCSP_LONGJMP
					delete [](it->second);
				#endif
				#ifdef CPPCSP_FIBERS
					DeleteFiber(it->second);
				#endif
					#ifdef CPPCSP_MSVC
						it = data->stacksToDelete.erase(it);
					#else
						data->stacksToDelete.erase(it++);
					#endif
				}
			}
			
			return true;
		}
		
		bool Kernel::AddProcess(KernelData* data,internal::ProcessPtr head,internal::ProcessPtr tail)
		{
			if (head == tail)
			{
				//This should be quicker than pushing a chain:
				data->runQueue.pushProcess(head);
			}
			else
			{
				data->runQueue.pushChain(head,tail);
			}
			return true;
		}
	}
}
