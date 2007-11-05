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

#include "test.h"
#include <boost/assign/list_of.hpp>

#include <boost/preprocessor.hpp>

#include "../src/cppcsp.h"
#include "../src/common/basic.h"



using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace boost::assign;
using namespace boost;

#include <list>

using namespace std;

template <> unsigned int volatile csp::internal::_AtomicProcessQueue<class csp::internal::Condition<1> >::ThreadsRunning;
template <> unsigned int volatile csp::internal::_AtomicProcessQueue<class csp::internal::MutexAndEvent<class csp::internal::OSBlockingMutex,1> >::ThreadsRunning;

namespace
{

class MutexTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;	

/*
	//Just us syncing:
	static TestResult test0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		Barrier barrier;
								
		BarrierEnd end(barrier.end());		
				
		//Test 0:
			end.enroll();
			end.sync();
			end.resign();			
		//End test 0
		
		ASSERTL(events.empty(),"Single-sync blocked",__LINE__);

		END_TEST("Barrier Test 0");
	}	
*/	
	static Time timeNonAtomicSwap(Mobile<BarrierEnd>& end)
	{
		Time start,finish,nonAtomic;		
		
		void* volatile p0 = NULL;
		void* volatile p1 = NULL;
		void* t = NULL;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			t = p1;
			p1 = p0;
			p0 = t;
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			t = p1;
			p1 = p0;
			p0 = t;
		}
		CurrentTime(&finish);
		
		end->sync();
		
		nonAtomic = finish;
		nonAtomic -= start;
		
		return nonAtomic;
	}
	
	static Time timeAtomicSwap(Mobile<BarrierEnd>& end)
	{
		Time start,finish,atomic;		
		
		void* volatile p0 = NULL;
		void* volatile p1 = NULL;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			p1 = AtomicSwap(&p0,p1);
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			p1 = AtomicSwap(&p0,p1);
		}
		CurrentTime(&finish);
		
		end->sync();
		
		atomic = finish - start;		
		
		return atomic;
	}
	
	static Time timeNonAtomicCmpSwap(Mobile<BarrierEnd>& end)
	{
		Time start,finish,nonAtomic;		
		
		void* volatile p0 = NULL;
		void* volatile p1 = NULL;
		void* t = NULL;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			if (p0 == NULL)
			{
				t = p1;
				p1 = p0;
				p0 = t;
			}			
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			if (p0 == NULL)
			{
				t = p1;
				p1 = p0;
				p0 = t;
			}
		}
		CurrentTime(&finish);
		
		end->sync();
		
		nonAtomic = finish - start;		
		
		return nonAtomic;
	}
	
	static Time timeAtomicCmpSwap(Mobile<BarrierEnd>& end)
	{
		Time start,finish,atomic;		
		
		void* volatile p0 = NULL;
		void* volatile p1 = NULL;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			p1 = AtomicCompareAndSwap(&p0,static_cast<void*>(NULL),p1);
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			p1 = AtomicCompareAndSwap(&p0,static_cast<void*>(NULL),p1);
		}
		CurrentTime(&finish);
		
		end->sync();
		
		atomic = finish - start;		
		
		return atomic;
	}
	
		
	static Time timeNonAtomicInc(Mobile<BarrierEnd>& end)
	{
		Time start,finish,nonAtomic;		
		
		volatile usign32 n = 0;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			n += 1;
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			n += 1;
		}
		CurrentTime(&finish);
		
		end->sync();
		
		nonAtomic = finish;
		nonAtomic -= start;
		
		return nonAtomic;
	}
	
	static Time timeAtomicInc(Mobile<BarrierEnd>& end)
	{
		Time start,finish,atomic;		
		
		volatile usign32 n = 0;
		
		//Get it into cache:
		for (int i = 0;i < 1000;i++)
		{
			AtomicIncrement(&n);
		}
		
		end->sync();
		
		CurrentTime(&start);
		for (int i = 0;i < 1000000 * 100;i++)
		{
			AtomicIncrement(&n);
		}
		CurrentTime(&finish);
		
		end->sync();
		
		atomic = finish - start;		
		
		return atomic;
	}

	static TestResult _atomicPerfTest(const char* name,Time (*nonAtomicFunc) (Mobile<BarrierEnd>&),Time (*atomicFunc) (Mobile<BarrierEnd>&))
	{
		//BufferedOne2OneChannel<Time> c0(Buffer<Time>::Factory(2)),c1(Buffer<Time>::Factory(2));
		BlackHoleChannel<Time> c;
		
		Barrier barrier;
		Time start,finish;
		Time times[5];
		
		{
			ScopedBarrierEnd end(barrier.end());		
			for (int i = 0;i < 3;i++)
			{
				ScopedForking forking;
		
				forking.fork(new EvaluateFunctionBarrier<Time>(i <= 1 ? nonAtomicFunc : atomicFunc,c.writer(),barrier.enrolledEnd()));
				forking.fork(new EvaluateFunctionBarrier<Time>(i <= 0 ? nonAtomicFunc : atomicFunc,c.writer(),barrier.enrolledEnd()));
	
				CurrentTime(&start);
				end.sync();
				end.sync();
				CurrentTime(&finish);		
			
				times[i] = finish - start;
			}			
		}	
		Mobile<BarrierEnd> end(barrier.enrolledEnd());
		
		times[3] = nonAtomicFunc (end);
		times[4] = atomicFunc (end);
				
		end->resign();
				
		return TestResultPass(string(name) + 
			": Non-Atomic Micros: " + lexical_cast<string>(GetSeconds(&(times[3])) / 100.0) + 
			": Atomic Micros: " + lexical_cast<string>(GetSeconds(&(times[4])) / 100.0) + 
			" Non-Atomic Micros (2 in par): " + lexical_cast<string>(GetSeconds(&(times[0])) / 100.0) + 
			" Half-Atomic Micros (2 in par): " + lexical_cast<string>(GetSeconds(&(times[1])) / 100.0) +
			" Atomic Micros (2 in par): " + lexical_cast<string>(GetSeconds(&(times[2])) / 100.0)		
		);
	}
	
	static TestResult atomicPerfTest0()
	{
		return _atomicPerfTest("Independent Cmp-Swap",timeNonAtomicCmpSwap,timeAtomicCmpSwap);
	}
	
	static TestResult atomicPerfTest1()
	{
		return _atomicPerfTest("Independent Swap",timeNonAtomicSwap,timeAtomicSwap);
	}
	
	static TestResult atomicPerfTest2()
	{
		return _atomicPerfTest("Independent Increment",timeNonAtomicInc,timeAtomicInc);
	}
		
	//TODO later performance test the above three with a shared variable
	
	
	template <typename MUTEXEND>
	class MutexClaimFunction
	{
		MUTEXEND mutexEnd;
		bool doYield;
	public:
		Time operator() (Mobile<BarrierEnd>& barrierEnd)
		{
			for (int i = 0;i < 100;i++)
			{
				mutexEnd.claim();
				mutexEnd.release();
			}
			
			barrierEnd->sync();
			for (int i = 0;i < (doYield ? 100 : 100000);i++)
			{
				mutexEnd.claim();
				if (doYield)
					CPPCSP_Yield();
				mutexEnd.release();
			}
			barrierEnd->sync();
			
			return Time();
		}
		
		inline MutexClaimFunction(MUTEXEND _mutexEnd,bool _doYield)
			:	mutexEnd(_mutexEnd),doYield(_doYield)
		{
		}
	};
	
	class TestProcess : public Process, private Primitive
	{			
	public:
		void runProcess() {};
		void endProcess() {};
	
		inline TestProcess()
			:	Process(GetKernel(),NULL,0)
		{
		}
	};
	
	template <typename CONDITION>
	class QueueIdFunction : private Primitive
	{
		_AtomicProcessQueue<CONDITION>* send;
		_AtomicProcessQueue<CONDITION>* recv;
		TestProcess* proc[10000];
	public:
		Time operator() (Mobile<BarrierEnd>& barrierEnd)
		{
			for (int i = 0;i < 10000;i++)
			{
				proc[i] = new TestProcess;
			}
			
			for (int i = 0;i < 100;i++)
			{
				send->pushProcess(proc[i]);
				recv->popHead();
			}
			
			barrierEnd->sync();
			for (int i = 0;i < 10000;i++)
			{
				send->pushProcess(proc[i]);
				recv->popHead();
			}
			barrierEnd->sync();
			
			for (int i = 0;i < 10000;i++)
			{
				delete proc[i];
			}
			
			return Time();
		}
		
		inline QueueIdFunction(_AtomicProcessQueue<CONDITION>* _send,_AtomicProcessQueue<CONDITION>* _recv)
			:	send(_send),recv(_recv)
		{
		}
	};
	
	template <typename MUTEX>
	static TestResult _mutexPerfTestDep(const char* name,int threads, int claimersPerThread,bool yield)
	{				
		BlackHoleChannel<Time> c;
		
		Barrier barrier;
		Time start,finish;
		Time time;
		
		list<ThreadCSProcessPtr> processes;
		MUTEX mutex;
		
		for (int i = 0;i < threads;i++)
		{			
			list<CSProcessPtr> subProcesses;
			for (int j = 0;j < claimersPerThread;j++)
			{
				subProcesses.push_back(new EvaluateFunctionBarrier<Time,MutexClaimFunction<typename MUTEX::End> >(MutexClaimFunction<typename MUTEX::End>(mutex.end(),yield),c.writer(),barrier.enrolledEnd()));
			}
			
			processes.push_back(InParallelOneThread(subProcesses.begin(),subProcesses.end()).process());
		}
		
		{
			ScopedBarrierEnd end(barrier.end());						
			
			ScopedForking forking;
		
			forking.fork(processes.begin(),processes.end());
	
			CurrentTime(&start);
			end.sync();
			end.sync();
			CurrentTime(&finish);		
			
			time = finish - start;			
		}	
		
		return TestResultPass(string(name) + " Mutex Test (" + lexical_cast<string>(threads) + " threads, each with " + lexical_cast<string>(claimersPerThread) + string(yield ? " yielding" : " empty") + " claimers): "
			+ lexical_cast<string>(GetSeconds(&time) * (yield ? 10000.0 : 10.0)) + " microseconds per claim/release");
	}
	
	template <typename MUTEX>
	static TestResult _mutexPerfTestInd(const char* name)
	{				
		BlackHoleChannel<Time> c;
		
		Barrier barrier;
		Time start,finish;
		Time time;
		
		list<ThreadCSProcessPtr> processes;
		MUTEX mutexA,mutexB;
		
		processes.push_back(new EvaluateFunctionBarrier<Time,MutexClaimFunction<typename MUTEX::End> >(MutexClaimFunction<typename MUTEX::End>(mutexA.end(),false),c.writer(),barrier.enrolledEnd()));
		processes.push_back(new EvaluateFunctionBarrier<Time,MutexClaimFunction<typename MUTEX::End> >(MutexClaimFunction<typename MUTEX::End>(mutexB.end(),false),c.writer(),barrier.enrolledEnd()));
				
		{
			ScopedBarrierEnd end(barrier.end());						
			
			ScopedForking forking;
		
			forking.fork(processes.begin(),processes.end());
	
			CurrentTime(&start);
			end.sync();
			end.sync();
			CurrentTime(&finish);		
			
			time = finish - start;			
		}	
		
		return TestResultPass(string(name) + " Mutex Test, Independent Mutexes (2 threads, each with 1 empty claimers): "
			+ lexical_cast<string>(GetSeconds(&time) * 100.0) + " microseconds per claim/release");
	}
	
#define BOTH_MUTEX_LIST (QueuedMutex) (SpinMutex) (PureSpinMutex) (PureSpinMutex_TTS) (OSNonBlockingMutex) (OSBlockingMutex)
#ifdef CPPCSP_WINDOWS
	#define MUTEX_LIST BOTH_MUTEX_LIST (OSCritSection) (OSNonBlockingCritSection)
#else
	#define MUTEX_LIST BOTH_MUTEX_LIST
#endif

#define MUTEX_TEST_NAME(r,params,mutex) BOOST_PP_CAT(mutexPerfTest_,BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2,0,params),BOOST_PP_CAT(_,BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2,1,params),mutex))))

#define MUTEX_TEST_NAME_BR(r,params,mutex) ( MUTEX_TEST_NAME(r,params,mutex) )

#define MUTEX_IND_TEST_NAME_BR(r,params,mutex) ( BOOST_PP_CAT(mutexPerfTestInd_,mutex) )

#define MUTEX_TEST(r,params,mutex) static TestResult MUTEX_TEST_NAME(r,params,mutex) () { \
	return _mutexPerfTestDep< mutex > ( BOOST_PP_STRINGIZE(mutex) , BOOST_PP_TUPLE_ELEM(2,0,params) , BOOST_PP_TUPLE_ELEM(2,1,params) , false ) ; }

#define MUTEX_TEST_IND(r,params,mutex) static TestResult BOOST_PP_CAT(mutexPerfTestInd_,mutex) () { \
	return _mutexPerfTestInd< mutex > ( BOOST_PP_STRINGIZE(mutex) ) ; }

	
	BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST,(2,1),MUTEX_LIST)
	
	BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST,(10,10),MUTEX_LIST)
	
	BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST,(1,1),MUTEX_LIST)
	
	BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST_IND,dummyparam,MUTEX_LIST)
	
	template <typename CONDITION>
	static TestResult _queuePerfTest(const std::string& name)
	{
		BlackHoleChannel<Time> c;
		
		Barrier barrier;
		Time start,finish;
		Time time;
		
		list<ThreadCSProcessPtr> processes;
		_AtomicProcessQueue<CONDITION> queueA,queueB;		
		
		processes.push_back(new EvaluateFunctionBarrier<Time,QueueIdFunction<CONDITION> >(QueueIdFunction<CONDITION>(&queueA,&queueB),c.writer(),barrier.enrolledEnd()));
		processes.push_back(new EvaluateFunctionBarrier<Time,QueueIdFunction<CONDITION> >(QueueIdFunction<CONDITION>(&queueB,&queueA),c.writer(),barrier.enrolledEnd()));
				
		{
			ScopedBarrierEnd end(barrier.end());						
			
			ScopedForking forking;
		
			forking.fork(processes.begin(),processes.end());
	
			CurrentTime(&start);
			end.sync();
			end.sync();
			CurrentTime(&finish);		
			
			time = finish - start;			
		}	
		
		return TestResultPass(string(name) + " Queue Test, Write-Read (2 threads): "
			+ lexical_cast<string>(GetSeconds(&time) * 100.0) + " microseconds per write-and-read");
	}
	
	static TestResult queuePerfTest0()
	{
	#ifdef CPPCSP_DARWIN
		return TestResultPass("Test not applicable on this platform");
	#else
		return _queuePerfTest< MutexAndEvent<OSBlockingMutex> >("Queue Test - Native Mutex and Native Event");
	#endif
	}
	
	static TestResult queuePerfTest1()
	{
	#ifdef CPPCSP_DARWIN
		return TestResultPass("Test not applicable on this platform");
	#else
		return _queuePerfTest< MutexAndEvent<PureSpinMutex> >("Queue Test - Pure-Spin Mutex and Native Event");
	#endif
	}
	
	static TestResult queuePerfTest2()
	{
		return _queuePerfTest< Condition<> >("Queue Test - Condition");
	}
	
	std::list<TestResult (*) ()> tests()
	{
		us = currentProcess();
		return list<TestResult (*) ()>(); //list_of<TestResult (*) ()>(); 
	}

	std::list<TestResult (*) ()> perfTests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>
			(atomicPerfTest0) (atomicPerfTest1) (atomicPerfTest2)
			BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST_NAME_BR,(2,1),MUTEX_LIST)
			BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST_NAME_BR,(10,10),MUTEX_LIST)
			BOOST_PP_SEQ_FOR_EACH(MUTEX_TEST_NAME_BR,(1,1),MUTEX_LIST)
			BOOST_PP_SEQ_FOR_EACH(MUTEX_IND_TEST_NAME_BR,dummyparam,MUTEX_LIST)
			(queuePerfTest0) (queuePerfTest1) (queuePerfTest2)
		; 
	}
	
	
	
/*	
	static TestResult mutexPerfTest1A()
	{
		return _mutexPerfTest1<QueuedMutex>("QueuedMutex",2,1,false);
	}
	
	static TestResult mutexPerfTest1B()
	{
		return _mutexPerfTest1<SpinMutex>("SpinMutex",2,1,false);
	}
	
	static TestResult mutexPerfTest1C()
	{
		return _mutexPerfTest1<PureSpinMutex>("PureSpinMutex",2,1,false);
	}
	
	static TestResult mutexPerfTest1D()
	{
		return _mutexPerfTest1<PureSpinMutex_TTS>("PureSpinMutex_TTS",2,1,false);
	}
	
	static TestResult mutexPerfTest1E()
	{
		return _mutexPerfTest1<OSNonBlockingMutex>("OSNonBlockingMutex",2,1,false);
	}
	
	static TestResult mutexPerfTest1F()
	{
		return _mutexPerfTest1<OSBlockingMutex>("OSBlockingMutex",2,1,false);
	}
		
	static TestResult mutexPerfTest1G()
	{
	#ifdef CPPCSP_WINDOWS
		return _mutexPerfTest1<OSCritSection>("OSCritSection",2,1,false);
	#else
		return TestResultPass("Test not present on this OS");
	#endif
	}
	
	
	static TestResult mutexPerfTest2A()
	{
		return _mutexPerfTest1<QueuedMutex>("QueuedMutex",10,10,false);
	}
	
	static TestResult mutexPerfTest2B()
	{
		return _mutexPerfTest1<SpinMutex>("SpinMutex",10,10,false);
	}
	
	static TestResult mutexPerfTest2C()
	{
		return _mutexPerfTest1<PureSpinMutex>("PureSpinMutex",10,10,false);
	}
	
	static TestResult mutexPerfTest2D()
	{
		return _mutexPerfTest1<PureSpinMutex_TTS>("PureSpinMutex_TTS",10,10,false);
	}
	
	static TestResult mutexPerfTest2E()
	{
		return _mutexPerfTest1<OSNonBlockingMutex>("OSNonBlockingMutex",10,10,false);
	}
	
	static TestResult mutexPerfTest2F()
	{
		return _mutexPerfTest1<OSBlockingMutex>("OSBlockingMutex",10,10,false);
	}
		
	static TestResult mutexPerfTest2G()
	{
	#ifdef CPPCSP_WINDOWS
		return _mutexPerfTest1<OSCritSection>("OSCritSection",10,10,false);
	#else
		return TestResultPass("Test not present on this OS");
	#endif
	}
	
	static TestResult mutexPerfTest3A()
	{
		return _mutexPerfTest1<QueuedMutex>("QueuedMutex",2,1,true);
	}
	
	static TestResult mutexPerfTest3B()
	{
		return _mutexPerfTest1<SpinMutex>("SpinMutex",2,1,true);
	}	
	
	static TestResult mutexPerfTest4A()
	{
		return _mutexPerfTest1<QueuedMutex>("QueuedMutex",10,10,true);
	}
	
	static TestResult mutexPerfTest4B()
	{
		return _mutexPerfTest1<SpinMutex>("SpinMutex",10,10,true);
	}	
*/
	inline virtual ~MutexTest()
	{
	}
};

ProcessPtr MutexTest::us;


}

Test* GetMutexTest()
{
	return new MutexTest;
}
