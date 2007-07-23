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
#include <boost/assign/std/set.hpp>

#include "../src/cppcsp.h"

#include "../src/common/basic.h"
#include "../src/common/barrier_bucket.h"

using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace boost::assign;
using namespace boost;

#include <list>

using namespace std;

namespace
{

class RunTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;

	//Run a simple empty process:
	static TestResult test0()
	{
		BEGIN_TEST()
	
		SetUp setup;
		
		CSProcess* _skip = new SkipProcess;
		ProcessPtr skip(getProcessPtr(_skip));
		
		EventList expA = tuple_list_of
			(us,skip,skip) //We add them to the queue
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(skip,us,us) (NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish, and free us from the barrier, then reschedule
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);
		
			RunInThisThread(_skip);			
		}
		
		ASSERTEQ(expA,actA,"Run Test 0 part A not as expected",__LINE__);

		END_TEST("Run Test 0");
	}


	//Fork a simple empty process:
	static TestResult test1()
	{
		BEGIN_TEST()
	
		SetUp setup;
		
		CSProcess* _skip = new SkipProcess;
		ProcessPtr skip(getProcessPtr(_skip));
		
		EventList expA = tuple_list_of
			(us,skip,skip) //We add them to the queue
		;
		
		EventList actA;
		
		ScopedForking forking;
		
		{
			RecordEvents _(&actA);
		
			forking.forkInThisThread(_skip);
		}
		
		EventList expB = tuple_list_of
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish and reschedule
		;		
		
		EventList actB;
		
		{
			RecordEvents _(&actB);
		
			CPPCSP_Yield();
		}		
		
		ASSERTEQ(expA,actA,"Run Test 1 part A not as expected",__LINE__);
		ASSERTEQ(expB,actB,"Run Test 1 part B not as expected",__LINE__);

		END_TEST("Run Test 1");
	}
	
	//Run a composite parallel process:
	static TestResult test2()
	{
		BEGIN_TEST()
		
		cerr << "STARTED TEST 2" << endl;
	
		SetUp setup;
		
		CSProcess* _skip0 = new SkipProcess;
		ProcessPtr skip0(getProcessPtr(_skip0));
		CSProcess* _skip1 = new SkipProcess;		
		ProcessPtr skip1(getProcessPtr(_skip1));
		CSProcess* _skip2 = new SkipProcess;
		ProcessPtr skip2(getProcessPtr(_skip2));				
		
		EventList expA = tuple_list_of
			(us,skip0,skip0) (us,skip1,skip1) (us,skip2,skip2) //They are added to the queue
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip0 finishes
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1 finishes
			(skip2,us,us) //skip2 frees us when it syncs on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip2 finishes
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);

			RunInThisThread(InParallelOneThread(_skip0)(_skip1)(_skip2));
		}
		
		
		ASSERTEQ(expA,actA,"Run Test 2 part A not as expected",__LINE__);


		END_TEST("Run Test 2");
	}	


	//Run a composite sequential process:
	static TestResult test3()
	{
		BEGIN_TEST()

		cerr << "STARTED TEST 3" << endl;

	
		SetUp setup;
		
		CSProcess* _skip0 = new SkipProcess;
		ProcessPtr skip0(getProcessPtr(_skip0));
		CSProcess* _skip1 = new SkipProcess;
		ProcessPtr skip1(getProcessPtr(_skip1));
		CSProcess* _skip2 = new SkipProcess;
		ProcessPtr skip2(getProcessPtr(_skip2));				
		
		EventList expA = tuple_list_of
			(us,skip0,skip0) //They are added to the queue			
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(skip0,us,us) //skip0 frees us when it syncs on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip0 finishes


			(us,skip1,skip1) //They are added to the queue			
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(skip1,us,us) //skip1 frees us when it syncs on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1 finishes
			
			(us,skip2,skip2) //They are added to the queue			
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(skip2,us,us) //skip2 frees us when it syncs on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip2 finishes			
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);

			RunInThisThread(InSequenceOneThread(_skip0)(_skip1)(_skip2));
		}
		
		
		ASSERTEQ(expA,actA,"Run Test 3 part A not as expected",__LINE__);


		END_TEST("Run Test 3");
	}	
	
	#define DECLARE_SKIP(label) CSProcess* _skip##label = new SkipProcess; ProcessPtr skip##label (getProcessPtr( _skip##label ));

	//Run a composite sequential/parallel process:
	static TestResult test4()
	{
		BEGIN_TEST()
	
		cerr << "STARTED TEST 4" << endl;
	
	
		SetUp setup;
		
		//SEQ 
		//  PAR (0)
		//    SEQ (0_0)
		//      skip0_0_0
		//      skip0_0_1
		//    skip0_1
		//  PAR (1)
		//    PAR (1_0)
		//      skip1_0_0
		//      skip1_0_1
		//    SEQ (1_1)
		//      skip1_1_0
		//      skip1_1_1
		
		DECLARE_SKIP(0_0_0)
		DECLARE_SKIP(0_0_1)
		DECLARE_SKIP(0_1)
		DECLARE_SKIP(1_0_0)
		DECLARE_SKIP(1_0_1)
		DECLARE_SKIP(1_1_0)
		DECLARE_SKIP(1_1_1)
		
		CSProcess* _seq0_0 = InSequenceOneThread(_skip0_0_0)(_skip0_0_1).process();
		CSProcess* _par0 = InParallelOneThread(_seq0_0)(_skip0_1).process();
		
		CSProcess* _par1_0 = InParallelOneThread(_skip1_0_0)(_skip1_0_1).process();
		CSProcess* _seq1_1 = InSequenceOneThread(_skip1_1_0)(_skip1_1_1).process();
		
		CSProcess* _par1 = InParallelOneThread(_par1_0)(_seq1_1).process();
		
		ProcessPtr par0 = getProcessPtr(_par0);
		ProcessPtr par1 = getProcessPtr(_par1);
		ProcessPtr par1_0 = getProcessPtr(_par1_0);

		ProcessPtr seq0_0 = getProcessPtr(_seq0_0);
		ProcessPtr seq1_1 = getProcessPtr(_seq1_1);

		
		EventList expA = tuple_list_of
			(us,par0,par0) //PAR 0 added to the queue
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier

			(par0,seq0_0,seq0_0) (par0,skip0_1,skip0_1) //Contents of PAR 0 added to the run queue
			(par0,NullProcessPtr,NullProcessPtr) //PAR 0 blocks on its barrier
			
			(seq0_0,skip0_0_0,skip0_0_0) //First process in SEQ 0_0 added to the run queue
			(seq0_0,NullProcessPtr,NullProcessPtr) //SEQ 0_0 blocks on its barrier
			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip0_1 finishes

			(skip0_0_0,seq0_0,seq0_0) //skip0_0_0 frees seq0_0 when it syncs on its barrier			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip0_0_0 finishes
			
			(seq0_0,skip0_0_1,skip0_0_1) //Second process in SEQ 0_0
			(seq0_0,NullProcessPtr,NullProcessPtr) //SEQ 0_0 blocks on its barrier

			(skip0_0_1,seq0_0,seq0_0) //skip0_0_1 frees seq0_0 when it syncs on its barrier			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip0_0_1 finishes			
			
			(seq0_0,par0,par0) //SEQ 0_0 frees PAR 0 when it syncs on its barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //seq0_0 finishes
			
			(par0,us,us) //PAR 0 frees us when it syncs on its barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //par0 finishes
			
			(us,par1,par1) //PAR 1 added to the queue
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			
			(par1,par1_0,par1_0) (par1,seq1_1,seq1_1) //contents of PAR 1 added to the queue
			(par1,NullProcessPtr,NullProcessPtr) //PAR 1 blocks on the barrier
			
			(par1_0,skip1_0_0,skip1_0_0) (par1_0,skip1_0_1,skip1_0_1) //contents of PAR 1_0 added to the queue 
			(par1_0,NullProcessPtr,NullProcessPtr) //PAR 1_0 blocks on the barrier
			
			(seq1_1,skip1_1_0,skip1_1_0) //First process in SEQ 1_1 added to the queue
			(seq1_1,NullProcessPtr,NullProcessPtr) //SEQ1_1 blocks on the barrier
						
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1_0_0 finishes
			
			(skip1_0_1,par1_0,par1_0) //skip1_0_1 frees par_1_0 when it syncs on its barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1_0_1 finishes
			
			(skip1_1_0,seq1_1,seq1_1) //skip1_1_0 frees seq1_1 when it syncs on its barrier			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1_1_0 finishe
			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //par1_0 finishes
			
			(seq1_1,skip1_1_1,skip1_1_1) //seq1_1 starts up its last process
			(seq1_1,NullProcessPtr,NullProcessPtr) //SEQ 1_1 blocks on the barrier
			
			(skip1_1_1,seq1_1,seq1_1) //skip1_1_1 frees seq1_1 when it syncs on the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //skip1_1_1 finishes
			
			(seq1_1,par1,par1) //SEQ1_1 syncs on its barrier, freeing par1
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //seq1_1 finishes
			
			(par1,us,us) //PAR1 syncs on its barrier, freeing us
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //PAR1 finishes
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);

			RunInThisThread(InSequenceOneThread(_par0)(_par1));
		}
		
		
		ASSERTEQ(expA,actA,"part A not as expected",__LINE__);


		END_TEST("Run Test 4");
	}	
	

	
	//Run a simple empty process as another thread:
	static TestResult test5()
	{
		BEGIN_TEST()
	
		SetUp setup;

		CSProcess* _skip = new SkipProcess;
		ProcessPtr skip(getProcessPtr(_skip));
		
				
		set<EventList> expA_0,expA_1;
		expA_0 += tuple_list_of
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
		;
		expA_0 += tuple_list_of
			(skip,us,us) //They finish, and free us from the barrier
		;
		//expA_1 is blank (they finish first, we never block)
		
		set<EventList> actA;
		{
			RecordEvents _(&actA);
						
			Run(_skip);
		}
				
		ASSERTEQ1OF2(expA_0,expA_1,actA,"part A not as expected",__LINE__);

		END_TEST("Run Test 5");
	}	
	
	//TODO add some tests testing running composite processes in a new thread
	
	class DummyException {};
	
	static TestResult testForkingException()
	{
		BEGIN_TEST()
	
		SetUp setup;
		
		CSProcess* _skip = new SkipProcess;
		ProcessPtr skip(getProcessPtr(_skip));
		
		EventList expA = tuple_list_of
			(us,skip,skip) //We add them to the queue
			(us,NullProcessPtr,NullProcessPtr) //We block on the barrier
			(skip,us,us) //They free us from the barrier
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
		;
		
		EventList actA;
				
		{
			RecordEvents _(&actA);
			
			try
			{
				ScopedForking forking;
		
				forking.forkInThisThread(_skip);
				
				throw DummyException();
			}
			catch (DummyException)
			{
			}						
		}
		
		ASSERTEQ(expA,actA,"part A not as expected",__LINE__);
				
		END_TEST("Forking Exception Test");
	}
		

	//TODO later test multi-threading more
	
	//TODO later test scoped forking some more
	
	static TestResult _limitTest(bool useThreads,unsigned stackSize,usign32 maxAttempt)
	{	
		usign32 numberOfThreads = 0;
		Time now,limit,finish,start;
		bool outOfResources = false;
		string reason;


		BEGIN_TEST()
		
		Barrier barrier;
		try
		{		
			ScopedForking forking;
		{
			ScopedBarrierEnd end(barrier.end());
				
			CurrentTime(&start);
			now = start;
			limit = Seconds(5); 
			limit += start;		
		
			
			
				while (numberOfThreads < maxAttempt && now < limit)
				{
					forking.fork(new BarrierSyncer(barrier.enrolledEnd(),1));
					numberOfThreads += 1;
					CurrentTime(&now);
				}			
			
				CurrentTime(&finish);
								
			//The sync lets everyone finish:
			end.sync();
		}
		}
		catch (OutOfResourcesException&)
		{
			CurrentTime(&finish);
			outOfResources = true;
		}		

		finish -= start;
				
		if (outOfResources)
		{
			reason = "Out of resources";
		}
		else
		{
			reason = "Out of time (5 seconds)";
		}
		
		
		END_TEST(string(useThreads ? "Kernel-Threads" : "User-Threads") + ", stack size:" + lexical_cast<string>(stackSize) + ", threads created: " + lexical_cast<string>(numberOfThreads) + ( (numberOfThreads < maxAttempt) ? (string(", stopped because: ") + reason) : "") + (string("; took (seconds): ") + lexical_cast<string>(GetSeconds(&finish)) ) );
	}

	//Each of these four tests attempts to use 0.5 gig of memory for stacks

	static TestResult limitTestUser4k()
	{
		//4k * 2^17 = 0.5 gig		
	
		return _limitTest(false,4096, 131072 );
	}
	
	static TestResult limitTestUser64k()
	{
		return _limitTest(false,65536, 8192 );
	}
	
	static TestResult limitTestKernel4k()
	{
		return _limitTest(true, 4096, 131072 );
	}
	
	static TestResult limitTestKernel64k()
	{
		return _limitTest(true, 65536, 8192 );
	}				
	
	std::list<TestResult (*)()> tests()
	{
		us = currentProcess();
		return list_of<TestResult (*)()>(test0)(test1)(test2)(test3)(test4)(test5) (testForkingException)
		;
	}
	
	std::list<TestResult (*)()> perfTests()
	{
		us = currentProcess();
		return std::list<TestResult (*)()>() /*list_of<TestResult (*)()>(limitTestUser4k) (limitTestUser64k) (limitTestKernel4k) (limitTestKernel64k) */		
		;
	}	
		
	inline virtual ~RunTest()
	{
	}
};

ProcessPtr RunTest::us;

} //namespace 

Test* GetRunTest()
{
	return new RunTest;
}
