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
#include <boost/lexical_cast.hpp>
#include "../src/cppcsp.h"
#include <math.h>

#include "../src/common/basic.h"
using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace std;
using namespace boost::assign;
using namespace boost;

namespace
{

//Never ready, yields on enable:
class YieldGuard : public Guard
{
protected:
	bool enable(internal::ProcessPtr)
	{
		CPPCSP_Yield();
		return false;
	}
	
	bool disable(internal::ProcessPtr)
	{
		return false;
	}
	
};

class TimeTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
	static int maxPrecision;

	//5% leniency for conversion
	inline static bool nearEqual(double a,double b)
	{
		return fabs(a - b) <= (fabs(a) * 0.05);
	}
	
	inline static bool nearEqual(const Time& a,const Time& b)
	{
		return nearEqual(GetSeconds(&a),GetSeconds(&b));
	}


	static TestResult test0()
	{
		BEGIN_TEST()
		
		double factors[] = {1, -1, 1.75, -1.75};
		
		for (int i = 0;i < 4;i++)
		{
			for (int mag = 3;mag > maxPrecision;mag -= 1)
			{
				Time t;
				double d = factors[i] * pow(10.0,static_cast<double>(mag)), d2;
				Seconds(d,&t);
				d2 = GetSeconds(&t);
		
				ASSERT(nearEqual(d,d2),"Time conversion not accurate for " + lexical_cast<string>(factors[i]) + " * 10^" + lexical_cast<string>(d));			
				
				if (mag > -3)
				{
					MilliSeconds(static_cast<long>(d * 1000.0),&t);
					d2 = GetSeconds(&t);
					ASSERT(nearEqual(d,d2),"Time conversion not accurate (millis) for " + lexical_cast<string>(factors[i]) + " * 10^" + lexical_cast<string>(d));
				}
				
				if (mag > -6)
				{
					MicroSeconds(static_cast<long>(d * 1000000.0),&t);
					d2 = GetSeconds(&t);
					ASSERT(nearEqual(d,d2),"Time conversion not accurate (micros) for " + lexical_cast<string>(factors[i]) + " * 10^" + lexical_cast<string>(d));
				}
			}
		}
		
		END_TEST("Time Test 0")
	}
	
	static void subtract(Time* a,Time* b,int method)
	{
		switch (method)
		{
			case 0:
				*a -= *b;
				break;
			case 1:
				*a = *a - *b;
				break;
			case 2:
				*a += Seconds(0) - *b;
				break;			
		}
	}

	static void add(Time* a,Time* b,int method)
	{
		switch (method)
		{
			case 0:
				*a += *b;
				break;
			case 1:
				*a = *a + *b;
				break;
			case 2:
				*a -= Seconds(0) - *b;
				break;			
		}
	}
	
	static TestResult test1()
	{
		BEGIN_TEST()
		
		double factors[] = {1, 1.5, 2, 5, -1, -1.5, -2, -5};
		
		for (int i = 0;i < 8;i++)
		{
			for (int j = 0;j < 8;j++)
			{
				//We stay one order of magnitude above the lowest precision to avoid representation errors
				for (int mag = 3;mag > maxPrecision;mag -= 1)
				{
					for (int mag2 = 3;mag2 > maxPrecision;mag2 -= 1)
					{
						for (int sub = 0;sub < 3;sub++)
						{
							Time t0,t1;
							double d0 = factors[i] * pow(10.0,static_cast<double>(mag));
							double d1 = factors[j] * pow(10.0,static_cast<double>(mag2));
							Seconds(d0,&t0);
							Seconds(d1,&t1);
							
							subtract(&t0,&t1,sub); //Leaves the result in t0							
		
							ASSERT(nearEqual(GetSeconds(&t0),d0 - d1),"Time conversion not accurate for subtraction method " + lexical_cast<string>(sub) +
								"; expected: " + lexical_cast<string>(d0 - d1) + " seconds but found: " + lexical_cast<string>(GetSeconds(&t0)) + " seconds");

							Seconds(d0,&t0);
							Seconds(d1,&t1);
							
							add(&t0,&t1,sub); //Leaves the result in t0							
		
							ASSERT(nearEqual(GetSeconds(&t0),d0 + d1),"Time conversion not accurate for addition method " + lexical_cast<string>(sub) +
								"; expected: " + lexical_cast<string>(d0 + d1) + " seconds but found: " + lexical_cast<string>(GetSeconds(&t0)) + " seconds");
								
							Seconds(d0,&t0);
							Seconds(d1,&t1);
							
							//Two of the lowest magnitude times may end up being equal in Time format
							//However, if two Times are different, the floating points must definitely be different
							if (t0 < t1)
							{
								ASSERT(d0 < d1, "Less-than not working correctly for " + lexical_cast<string>(d0) + " less than " + lexical_cast<string>(d1));
							}
							else if (t0 > t1)
							{
								ASSERT(d0 > d1, "Greater-than not working correctly for " + lexical_cast<string>(d0) + " greater than " + lexical_cast<string>(d1));
							}
						}
					}
				}
			}
		}
		
		END_TEST("Time Test 1")
	}
	
	//test truncation of times beyond possible precision:
	static TestResult test2()
	{
		BEGIN_TEST()
		
		double smallTime = pow(10.0,maxPrecision - 2);
		
		for (int mag = 3;mag >= maxPrecision;mag -= 1)
		{
			for (int factor = -1;factor <= 1;factor += 2) //To do both -1 and 1
			{
				double d = static_cast<double>(factor) * pow(10.0,static_cast<double>(mag));
			
				Time t,tsmaller,tlarger;
			
				Seconds(d,&t);
				Seconds(d - smallTime,&tsmaller);
				Seconds(d + smallTime,&tlarger);
			
				if (factor > 0)
				{
					//Positive number:
					
					//The larger number should be truncated:
					ASSERT(t == tlarger,"Positive number (...0001) was not truncated properly, magnitude: " + lexical_cast<string>(mag));
					
					//The smaller number should be truncated to the next one down:
					ASSERT(t >= tsmaller,"Positive number (...9999) was not truncated properly, magnitude: " + lexical_cast<string>(mag));
				}
				else if (factor < 0)
				{
					//Negative number:
					
					//The larger absolute number should be truncated:
					ASSERT(t == tsmaller,"Negative number (-...0001) was not truncated properly, magnitude: " + lexical_cast<string>(mag) + ", t: " + lexical_cast<string>(GetSeconds(t)) + ", tsmaller: " + lexical_cast<string>(GetSeconds(tsmaller)));
					
					//The smaller absolute number should be truncated to the next one nearest zero:
					ASSERT(t <= tlarger,"Negative number (-...9999) was not truncated properly, magnitude: " + lexical_cast<string>(mag));
				}
			}
		}
		
		END_TEST("Time Test 2")
	}
		
	//test getmilliseconds:
	
	static TestResult test3()
	{
		BEGIN_TEST()
		
		Time t;
		
		MilliSeconds(1,&t);		
		ASSERTEQ(GetMilliSeconds(t),1,"GetMilliSeconds fails for: 1",__LINE__);
		
		MilliSeconds(-1,&t);		
		ASSERTEQ(GetMilliSeconds(t),-1,"GetMilliSeconds fails for: -1",__LINE__);
		
		MicroSeconds(900,&t);		
		ASSERTEQ(GetMilliSeconds(t),0,"GetMilliSeconds fails for: 0.9",__LINE__);
		
		MicroSeconds(1100,&t);		
		ASSERTEQ(GetMilliSeconds(t),1,"GetMilliSeconds fails for: 1",__LINE__);
		
		MicroSeconds(-900,&t);		
		ASSERTEQ(GetMilliSeconds(t),0,"GetMilliSeconds fails for: -0.9",__LINE__);
		
		MicroSeconds(-1100,&t);		
		ASSERTEQ(GetMilliSeconds(t),-1,"GetMilliSeconds fails for: -1.1",__LINE__);
		
		END_TEST("Time Test 3")
	}	
	
	static ProcessPtr us;
	
	static TestResult test4()
	{
		BEGIN_TEST()
	
		SetUp setup;
		
		ScopedForking forking;
		
		CSProcess* _skip = new SkipProcess;
		ProcessPtr skip(getProcessPtr(_skip));
		
		EventList expA = tuple_list_of
			(us,skip,skip) //We add them to the queue
			//We never yield though			
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_skip);
			
			SleepFor(Seconds(0));						
			
			SleepUntil(CurrentTime());
		}
		
		ASSERTEQ(expA,actA,"Events were not as expected",__LINE__);
		
		Time t0 = CurrentTime();
		SleepFor(MilliSeconds(10));
		Time t1 = CurrentTime();
		
		ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough, SleepFor",__LINE__);
		t0 = CurrentTime() + MilliSeconds(10);
		SleepUntil(t0);
		ASSERTL(CurrentTime() >= t0,"Did not wait for long enough, SleepUntil",__LINE__);
		
		END_TEST("CPPCSP_SleepFor/CPPCSP_SleepUntil test");
	}
	
	static TestResult test5()
	{
		BEGIN_TEST()
		
		SetUp setup;
			
		Time t0,t1;
		
		t0 = CurrentTime();		
		RunInThisThread(new SleepForProcess(MilliSeconds(10)));
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		
		
		t0 = CurrentTime();		
		RunInThisThread(InParallelOneThread 
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(20)))
			(new SleepForProcess(MilliSeconds(30)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(30),"Did not wait for long enough",__LINE__);
		
		t0 = CurrentTime();		
		RunInThisThread(InSequenceOneThread
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(20)))
			(new SleepForProcess(MilliSeconds(30)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(60),"Did not wait for long enough",__LINE__);		
				
		t0 = CurrentTime();		
		RunInThisThread(InParallelOneThread 
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(10)))
			(new SleepForProcess(MilliSeconds(10)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		

		t0 = CurrentTime() + MilliSeconds(10);
		RunInThisThread(new SleepUntilProcess(t0));		
		t1 = CurrentTime();
		ASSERTL(t1 >= t0,"Did not wait for long enough",__LINE__);
		
		t0 = CurrentTime();		
		RunInThisThread(InParallelOneThread 
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(20)))
			(new SleepUntilProcess(t0 + MilliSeconds(30)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(30),"Did not wait for long enough",__LINE__);
		
		t0 = CurrentTime();		
		RunInThisThread(InSequenceOneThread 
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(20)))
			(new SleepUntilProcess(t0 + MilliSeconds(30)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(30),"Did not wait for long enough",__LINE__);		
		
		t0 = CurrentTime();		
		RunInThisThread(InSequenceOneThread 
			(new SleepUntilProcess(t0 + MilliSeconds(30)))
			(new SleepUntilProcess(t0 + MilliSeconds(20)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(30),"Did not wait for long enough",__LINE__);		

		t0 = CurrentTime();		
		RunInThisThread(InParallelOneThread 
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
			(new SleepUntilProcess(t0 + MilliSeconds(10)))
		);
		t1 = CurrentTime();
		ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		

		t0 = CurrentTime();
		RunInThisThread(new SleepUntilProcess(t0));		
		t1 = CurrentTime();
		ASSERTL(t1 >= t0,"Did not wait for long enough",__LINE__);
		
		t0 = CurrentTime();
		RunInThisThread(new SleepForProcess(MilliSeconds(1)));
		RunInThisThread(new SleepUntilProcess(t0));
		t1 = CurrentTime();
		ASSERTL(t1 >= t0,"Did not wait for long enough",__LINE__);		
		
		t0 = CurrentTime();
		RunInThisThread(new SleepForProcess(MilliSeconds(0)));
		t1 = CurrentTime();
		ASSERTL(t1 >= t0,"Did not wait for long enough",__LINE__);
		
		t0 = CurrentTime();
		RunInThisThread(new SleepForProcess(Seconds(-1)));
		t1 = CurrentTime();
		ASSERTL(t1 >= t0,"Did not wait for long enough",__LINE__);		
				
		END_TEST("Extensive SleepFor/SleepUntil test");
	}
	
	static TestResult test6()
	{
		BEGIN_TEST()
		
		Time t0,t1;
		unsigned n;
		
		{
			list<Guard*> guards = list_of<Guard*>
				(new RelTimeoutGuard(MilliSeconds(10)))				
				(new RelTimeoutGuard(MilliSeconds(10)))
				(new RelTimeoutGuard(MilliSeconds(10)))
			; 
		
			csp::Alternative alt(guards);
		
			t0 = CurrentTime();
			n = alt.priSelect();
			t1 = CurrentTime();
			
			//Given that we are pri-alting, and we know that the guards are enabled in order,
			//the first guard must be selected:
			
			ASSERTEQ(0,n,"First guard not selected",__LINE__);
			ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
			
			//Use the alt again -- should still wait for relative time:
			
			t0 = CurrentTime();
			n = alt.priSelect();
			t1 = CurrentTime();
			ASSERTEQ(0,n,"First guard not selected",__LINE__);
			ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		}
		
		{
			list<Guard*> guards = list_of<Guard*>
				(new RelTimeoutGuard(MilliSeconds(10)))				
				(new RelTimeoutGuard(MilliSeconds(20)))
				(new RelTimeoutGuard(MilliSeconds(30)))
			; 
		
			csp::Alternative alt(guards);
		
			t0 = CurrentTime();
			n = alt.priSelect();
			t1 = CurrentTime();
			
			//Given that we are pri-alting, and we know that the guards are enabled in order,
			//the first guard must be selected:
			
			ASSERTEQ(0,n,"First guard not selected",__LINE__);
			ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		}
		
		{
			list<Guard*> guards = list_of<Guard*>
				(new RelTimeoutGuard(MilliSeconds(30)))				
				(new RelTimeoutGuard(MilliSeconds(20)))
				(new RelTimeoutGuard(MilliSeconds(10)))
			; 
		
			csp::Alternative alt(guards);
		
			t0 = CurrentTime();
			n = alt.priSelect();
			t1 = CurrentTime();
			
			ASSERTL((t1 - t0) >= MilliSeconds(10),"Did not wait for long enough",__LINE__);
		}		


		//Test that yielding doesn't screw things up:
		{
			list<Guard*> guards = list_of<Guard*>
				(new RelTimeoutGuard(MilliSeconds(0)))
				(new YieldGuard)
				(new RelTimeoutGuard(MilliSeconds(10000)))
			; 
		
			csp::Alternative alt(guards);
		
			t0 = CurrentTime();
			n = alt.priSelect();
			t1 = CurrentTime();
			
			ASSERTL((t1 - t0) < MilliSeconds(10000),"Did not wait for long enough",__LINE__);
		}		
		
		END_TEST("Timeouts in ALT test");
	}

	static TestResult testAccuracy()
	{
		vector<double> diff(1000);
	
		for (int i = 0;i < 1000;i++)
		{
			csp::Time t,t2;			
			
			CurrentTime(&t);
			do
			{
				CurrentTime(&t2);
			}
			while (t == t2);
			
			t2 -= t; //get the difference between them
			
			diff[i] = GetSeconds(&t2);
		}
		
		//only take the lowest 80% of results, to remove the ones where we were pre-empted in the middle of the experiment:
		
		sort(diff.begin(),diff.end());
		
		double totalDiff = 0;
		
		for (int i = 0;i < 800;i++)
		{
			totalDiff += diff[i];
		}
		
		return TestResultPass("Rough average minimum timer accuracy: " + 
			lexical_cast<string>(totalDiff * (1000000000.0 / 800.0)) + //1 billion for nanoseconds, div by 800 for iterations
			" nanoseconds");
	}
	
	
	static int _testPrecision()
	{
		double seconds = 10.0; //ten seconds to start with
		int mag = 1;
		double r;
		
		do
		{
			seconds /= 10.0;
			mag -= 1;
			
			Time t;
			Seconds(seconds,&t);
			r = GetSeconds(&t);
		}
		while (fabs(r - seconds) < seconds * 0.1); //Allow 10% error for conversion
		
		mag += 1; //must be the one before this that was accurate
		
		return mag;
	}
	static TestResult testPrecision()
	{	
		return TestResultPass("Rough granularity of csp::Time: 10^" + lexical_cast<string>(_testPrecision()) + 
			" seconds (10^-3 = milli, 10^-6 = micro, 10^-9 = nano)");
	}

	std::list<TestResult (*)()> tests()
	{		
		maxPrecision = _testPrecision();
		us = currentProcess();
	
		//Tests 2 and 3 rely on certain floating point behaviour that is not portable,
		//so tests 2 and 3 are no longer run
		return list_of<TestResult (*)()>(test0)(test1)(test4)(test5)(test6)
		;
	}
	
	std::list<TestResult (*)()> perfTests()
	{
		us = currentProcess();
		return list_of<TestResult (*)()>(testAccuracy) (testPrecision)
		;
	}
};

int TimeTest::maxPrecision;
ProcessPtr TimeTest::us;

} //namespace



Test* GetTimeTest()
{
	return new TimeTest;
}
