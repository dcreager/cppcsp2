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

using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace boost::assign;
using namespace boost;

#include <list>

using namespace std;

class AltTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;

	std::list<TestResult (*) ()> tests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>(test0)(test1)(test2)(test3)(test4); 
	}

	std::list<TestResult (*) ()> perfTests()
	{
		us = currentProcess();		
		return list<TestResult (*) ()>(); //list_of<TestResult (*) ()>(); 
	}
	
	/**
	*	A guard used for testing.  It can be fired at any time, and records which process is doing
	*	the alting, and whether this guard was chosen in the alt
	*/
	class TestGuard : public Guard, private internal::Primitive
	{
		bool fired;
		AltingProcessPtr process;
		Guard** result;
	protected:
		bool enable(AltingProcessPtr _process)
		{			
			process = _process;
			return fired;
		}
		
		bool disable(AltingProcessPtr)
		{
			bool _fired = fired;
			fired = false;
			process = NullProcessPtr;
			return _fired;
		}
		
		void activate()
		{
			*result = this;
		}
	public:
		inline TestGuard(Guard** _result)
			:	fired(false),process(NullProcessPtr),result(_result)
		{
		}
	
		void fire()
		{
			fired = true;
			freeProcessMaybe(process);
		}
		
		friend class AltTest;
	};		
	
	//create a guard with a slow (i.e. event-waiting) enable, to test multi-threaded alts,
	//and see if an alt interrupted during its enable sequence gets added back to the run queue:
	
	class WaitingGuard : public TestGuard
	{
	#ifdef CPPCSP_DARWIN
		Condition<> cond;
	#else
		MutexAndEvent<OSBlockingMutex> cond;
	#endif
		bool waiting;
		bool waitDuringEnable;
	protected:
		bool enable(AltingProcessPtr _ptr)
		{
			if (waitDuringEnable)
			{
				cond.claim();
				waiting = true;
				cond.releaseWait();
			}
			return TestGuard::enable(_ptr);
		}
		
		bool disable(AltingProcessPtr _ptr)
		{
			if (false == waitDuringEnable)
			{
				cond.claim();
				waiting = true;
				cond.releaseWait();
			}
			return TestGuard::disable(_ptr);
		}
	public:
		inline WaitingGuard(Guard** _result, bool _waitDuringEnable /*otherwise, wait during disable*/)
			:	TestGuard(_result),waiting(false),waitDuringEnable(_waitDuringEnable)
		{
		}
		
		//Wait until they are waiting
		inline void waitForWait()
		{
			cond.claim();
			while (waiting == false)
			{
				cond.release();
				Thread_Yield();
				cond.claim();
			}
			cond.release();
		}
		
		inline void free()
		{
			cond.claim();
			cond.signal();
			cond.release();
		}
	};
	
	/**
	*	A function-like object for helping to test alts
	*/
	class TestAlt
	{
	public:
		//The ordered list of guards to alt over
		vector<TestGuard*> guards;		
		//The barrier end to sync twice on which we're finished:
		Mobile<BarrierEnd> barrierEnd;
	
		//Returns the guard the alt said it selected
		unsigned operator()()
		{
			barrierEnd->enroll();
		
			Alternative alt(guards.begin(),guards.end());
		
			unsigned sel = alt.priSelect();			
			
			//Let the test finish up:			
			
			barrierEnd->sync();
			barrierEnd->sync();
			
			barrierEnd->resign();
		
			return sel;
		}
		
		inline TestAlt(const Mobile<BarrierEnd>& _barrierEnd)
			:	barrierEnd(_barrierEnd)
		{
		}
	};
	
	///Checks that all four guards of an alt are unused
	static void assertAllGuardsEmpty(TestAlt* talt)
	{
		ASSERTEQ(NullProcessPtr,talt->guards[0]->process,"Data (0.process) not as expected",__LINE__);
		ASSERTEQ(NullProcessPtr,talt->guards[1]->process,"Data (1.process) not as expected",__LINE__);
		ASSERTEQ(NullProcessPtr,talt->guards[2]->process,"Data (2.process) not as expected",__LINE__);
		ASSERTEQ(NullProcessPtr,talt->guards[3]->process,"Data (3.process) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[0]->fired,"Data (0.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[1]->fired,"Data (1.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[2]->fired,"Data (2.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[3]->fired,"Data (3.fired) not as expected",__LINE__);
	}
	
	/**
	*	Tests alts blocking when no guards are ready, and then awaking (and finishing correctly) when a guard fires
	*/
	static TestResult test0()
	{
		BEGIN_TEST()
		
		SetUp setup;		
		
		ScopedForking forking;
		
		BufferedOne2OneChannel<unsigned> c(FIFOBuffer<unsigned>::Factory(1));
		
		Barrier barrier;
		ScopedBarrierEnd barrierEnd(barrier.enrolledEnd());
		
		TestAlt* talt = new TestAlt(barrier.enrolledEnd());
		
		Guard* result = NULL;
		
		talt->guards.resize(4);
		
		for (unsigned i = 0;i < 4;i++)
		{
			talt->guards[i] = new TestGuard(&result);
		}
		
		EvaluateFunction< unsigned, TestAlt >* _alter = new EvaluateFunction< unsigned, TestAlt >(*talt,c.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
		
		EventList expA = tuple_list_of
			(us,alter,alter) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(alter,NullProcessPtr,NullProcessPtr) //they must block, waiting for the guards
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Events not as expected in part A",__LINE__);
		ASSERTEQ(alter,talt->guards[0]->process,"Data (0.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[1]->process,"Data (1.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[2]->process,"Data (2.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[3]->process,"Data (3.process) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[0]->fired,"Data (0.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[1]->fired,"Data (1.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[2]->fired,"Data (2.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[3]->fired,"Data (3.fired) not as expected",__LINE__);
		
		EventList expB = tuple_list_of
			(us,alter,alter) //We fire guard[3]
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(alter,NullProcessPtr,NullProcessPtr) //they block on the barrier
		;
		
		EventList actB;
		
		{
			RecordEvents _(&actB);
		
			talt->guards[3]->fire();			
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Events not as expected in part B",__LINE__);	
		assertAllGuardsEmpty(talt);
		
		barrierEnd.sync();
		barrierEnd.sync();
		
		unsigned sel;
		c.reader() >> sel;
		
		ASSERTEQ(3,sel,"Alternative did not select the correct guard",__LINE__);
		ASSERTEQ(talt->guards[3],result,"Alternative did not activate the correct guard",__LINE__);
			
		delete talt;
			
		END_TEST("Alt Test 0");
	}
	
	/**
	*	Tests alts blocking when no guards are ready, and then awaking *once* when multiple guards fire
	*/
	static TestResult test1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		ScopedForking forking;
		
		BufferedOne2OneChannel<unsigned> c(FIFOBuffer<unsigned>::Factory(1));
		
		Barrier barrier;
		ScopedBarrierEnd barrierEnd(barrier.enrolledEnd());
		
		TestAlt* talt = new TestAlt(barrier.enrolledEnd());
		
		Guard* result = NULL;
		
		talt->guards.resize(4);
		
		for (unsigned i = 0;i < 4;i++)
		{
			talt->guards[i] = new TestGuard(&result);
		}
		
		EvaluateFunction< unsigned, TestAlt >* _alter = new EvaluateFunction< unsigned, TestAlt >(*talt,c.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
		
		EventList expA = tuple_list_of
			(us,alter,alter) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(alter,NullProcessPtr,NullProcessPtr) //they must block, waiting for the guards
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Events not as expected in part A",__LINE__);
		ASSERTEQ(alter,talt->guards[0]->process,"Data (0.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[1]->process,"Data (1.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[2]->process,"Data (2.process) not as expected",__LINE__);
		ASSERTEQ(alter,talt->guards[3]->process,"Data (3.process) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[0]->fired,"Data (0.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[1]->fired,"Data (1.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[2]->fired,"Data (2.fired) not as expected",__LINE__);
		ASSERTEQ(false,talt->guards[3]->fired,"Data (3.fired) not as expected",__LINE__);
		
		EventList expB = tuple_list_of
			(us,alter,alter) //We fire guard[3].
			//Firing guards[2] and guards[1] have no effect on the run-queue
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(alter,NullProcessPtr,NullProcessPtr) //they block on the barrier
		;
		
		EventList actB;
		
		{
			RecordEvents _(&actB);
		
			talt->guards[3]->fire();			
			talt->guards[2]->fire();
			talt->guards[1]->fire();
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Events not as expected in part B",__LINE__);	
		assertAllGuardsEmpty(talt);
		
		barrierEnd.sync();
		barrierEnd.sync();
		
		unsigned sel;
		c.reader() >> sel;
		
		ASSERTEQ(1,sel,"Alternative did not select the correct guard",__LINE__);
		ASSERTEQ(talt->guards[1],result,"Alternative did not activate the correct guard",__LINE__);
		
		delete talt;		
		
		END_TEST("Alt Test 1");
	}
		
	/**
	*	Tests the situation in an enabling sequence where the alt enables guard A,
	*	then while it is enabling guard B (A < B), guard A fires.  Even though the guard
	*	has already been dealt, the alt should notice, and not block at the end of its enable
	*/
	static TestResult test2()
	{
		BEGIN_TEST()
		
		SetUp setup;		
		
		BufferedOne2OneChannel<unsigned> c(FIFOBuffer<unsigned>::Factory(1));
		
		WaitingGuard* guard1;
		WaitingGuard* guard2;
		
		Barrier barrier;
		ScopedBarrierEnd barrierEnd(barrier.enrolledEnd());
		
		TestAlt* talt = new TestAlt(barrier.enrolledEnd());
		
		Guard* result = NULL;
		
		
		talt->guards.resize(4);		
		talt->guards[0] = new TestGuard(&result);
		talt->guards[1] = guard1 = new WaitingGuard(&result,true);
		talt->guards[2] = guard2 = new WaitingGuard(&result,true);
		talt->guards[3] = new TestGuard(&result);
		
		
		EvaluateFunction< unsigned, TestAlt >* _alter = new EvaluateFunction< unsigned, TestAlt >(*talt,c.writer());
    	ProcessPtr alter(getProcessPtr(_alter));		
		
		
		ScopedForking forking;
			
			set<EventList> expA_0,expA_1;
			//expA_0 is blank (the version where they don't block on the barrier)
		
			expA_1 += tuple_list_of			
				(alter,NullProcessPtr,NullProcessPtr) //They block on the barrier
			;
		
			set<EventList> actA;
		
			{
				RecordEvents _(&actA);			
			
				forking.fork(_alter);
						
				guard1->waitForWait();
				
				talt->guards[0]->fire();
				
				guard1->free();			
				
				guard2->waitForWait();	
				guard2->free();							
			}
		
		barrierEnd.sync();
		
		ASSERTEQ1OF2(expA_0,expA_1,actA,"Events not as expected in part A",__LINE__);
		assertAllGuardsEmpty(talt);
		
		barrierEnd.sync();
		
		unsigned sel;
		c.reader() >> sel;
		
		ASSERTEQ(0,sel,"Alternative did not select the correct guard",__LINE__);
		ASSERTEQ(talt->guards[0],result,"Alternative did not activate the correct guard",__LINE__);		
		
		END_TEST("Alt test 2");
		
	}
	
	/**
	*	Tests the situation in which the alt disable sequence is running, and has reached guard B,
	*	where some guard C (B < C) had fired before the alt began.  Guard A (A < B < C) is then fired.		
	*/
	static TestResult test3()
	{
		BEGIN_TEST()
		
		SetUp setup;		
		
		BufferedOne2OneChannel<unsigned> c(FIFOBuffer<unsigned>::Factory(1));
		
		WaitingGuard* guard1;		
		
		Barrier barrier;
		ScopedBarrierEnd barrierEnd(barrier.enrolledEnd());
		
		TestAlt* talt = new TestAlt(barrier.enrolledEnd());
		
		Guard* result = NULL;
		
		
		talt->guards.resize(4);		
		talt->guards[0] = new TestGuard(&result);
		talt->guards[1] = guard1 = new WaitingGuard(&result,false);
		talt->guards[2] = new TestGuard(&result);
		talt->guards[3] = new TestGuard(&result);
		
		
		EvaluateFunction< unsigned, TestAlt >* _alter = new EvaluateFunction< unsigned, TestAlt >(*talt,c.writer());
    	ProcessPtr alter(getProcessPtr(_alter));		
		
		{
			ScopedForking forking;
			
			set<EventList> expA_0,expA_1;			
			//In 0, they don't block on the barrier so it is blank
					
			expA_1 += tuple_list_of			
				(alter,NullProcessPtr,NullProcessPtr) //They block on the barrier
			;
		
			set<EventList> actA;
		
			{
				RecordEvents _(&actA);			
			
				//Guard is already fired:
				talt->guards[2]->fire();
			
				forking.fork(_alter);				
				
				guard1->waitForWait();
				
				talt->guards[0]->fire();
				
				guard1->free();					
			}
		
			barrierEnd.sync();			
			
			assertAllGuardsEmpty(talt);
			
			barrierEnd.sync();
			
			unsigned sel;
			c.reader() >> sel;
		
			ASSERTEQ1OF2(expA_0,expA_1,actA,"Events not as expected in part A",__LINE__);
			ASSERTEQ(0,sel,"Alternative did not select the correct guard",__LINE__);
			ASSERTEQ(talt->guards[0],result,"Alternative did not activate the correct guard",__LINE__);		
		
		}
		
		END_TEST("Alt test 3");
		
	}
	
	/**
	*	Tests the situation in which the alt disable sequence is running, and has reached guard B,
	*	where some guard C (B < C) had fired after all guards had been enabled.  Guard A (A < B < C) is then fired.		
	*/
	static TestResult test4()
	{
		BEGIN_TEST()
		
		SetUp setup;		
		
		BufferedOne2OneChannel<unsigned> c(FIFOBuffer<unsigned>::Factory(1));
		
		WaitingGuard* guard1;		
		
		Barrier barrier;
		ScopedBarrierEnd barrierEnd(barrier.enrolledEnd());
		
		TestAlt* talt = new TestAlt(barrier.enrolledEnd());
		
		Guard* result = NULL;
		
		
		talt->guards.resize(4);		
		talt->guards[0] = new TestGuard(&result);
		talt->guards[1] = guard1 = new WaitingGuard(&result,false);
		talt->guards[2] = new TestGuard(&result);
		talt->guards[3] = new TestGuard(&result);
		
		
		EvaluateFunction< unsigned, TestAlt >* _alter = new EvaluateFunction< unsigned, TestAlt >(*talt,c.writer());
    	ProcessPtr alter(getProcessPtr(_alter));		
		
		CPPCSP_Yield();
		
		{
			ScopedForking forking;
			
			set<EventList> expA_0,expA_1;			
			expA_0 += tuple_list_of	
				(alter,NullProcessPtr,NullProcessPtr) //They block on the barrier				
			;
			expA_0 += tuple_list_of	
				(us,alter,alter) //We free them
			;
									
			//In 0, they don't block on the barrier 
					
			expA_1 += tuple_list_of	
				(alter,NullProcessPtr,NullProcessPtr) //They block on the alt				
				(alter,NullProcessPtr,NullProcessPtr) //They block on the barrier
			;
			expA_1 += tuple_list_of	
				(us,alter,alter) //We free them
			;
		
			set<EventList> actA;
		
			{
				RecordEvents _(&actA);			
			
				//Guard is already fired:				
							
				forking.fork(_alter);
				
				while (AtomicGet(&(alter->alting)) != ___ALTING_WAITING)
				{
					Thread_Yield();
				}
				
				talt->guards[2]->fire();
				
				guard1->waitForWait();
				
				talt->guards[0]->fire();
				
				guard1->free();					
			}
		
			barrierEnd.sync();
			
			ASSERTEQ1OF2(expA_0,expA_1,actA,"Events not as expected in part A",__LINE__);
			assertAllGuardsEmpty(talt);
			
			barrierEnd.sync();
			
			unsigned sel;
			c.reader() >> sel;
		
			ASSERTEQ(0,sel,"Alternative did not select the correct guard",__LINE__);
			ASSERTEQ(talt->guards[0],result,"Alternative did not activate the correct guard",__LINE__);		
		
		}
		
		END_TEST("Alt test 4");
		
	}


	inline virtual ~AltTest()
	{
	}
};

ProcessPtr AltTest::us;


Test* GetAltTest()
{
	return new AltTest;
}
