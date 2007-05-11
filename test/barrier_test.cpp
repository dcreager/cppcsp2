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

#include "../src/cppcsp.h"
#include "../src/common/barrier_bucket.h"

using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace boost::assign;
using namespace boost;

#include <list>

using namespace std;

SchedulerRecorder::ThreadedEventList SchedulerRecorder::events;
PureSpinMutex SchedulerRecorder::mutex;


template <typename MUTEX>
class SimpleBarrier : public BarrierBase, private internal::Primitive
{
private:
	MUTEX mutex;
	volatile int enrolled;
	volatile int leftToSync;
	map<ThreadId, pair<ProcessPtr,ProcessPtr> > queues;
	
	inline bool _sync()
	{
		if (--leftToSync <= 0)
		{
			
		
			for (map<ThreadId, pair<ProcessPtr,ProcessPtr> >::iterator it = queues.begin();it != queues.end();it++)
			{
				freeProcessChain(it->second.first,it->second.second);
				it->second.first = NullProcessPtr;
				it->second.second = NullProcessPtr;
			}
			
			queues.clear();
			
			leftToSync = enrolled;
			return true;
		}
		else
		{
			return false;
		}
	}
public:
	inline SimpleBarrier() : enrolled(0),leftToSync(0)
	{
	}

	virtual void* enroll()
	{
		typename MUTEX::End end(mutex.end());
		end.claim();
		enrolled++;
		leftToSync++;
		end.release();
		return &queues;
	}
			
	virtual void halfEnroll()
	{
		enroll();
	}
			
	virtual void* completeEnroll()
	{
		return &queues;
	}
			
	virtual void resign(void*)
	{
		typename MUTEX::End end(mutex.end());
		end.claim();
		enrolled--;
		_sync();
		end.release();
	}
			
	virtual void sync(void*)
	{		
		typename MUTEX::End end(mutex.end());
		end.claim();
		if (false == _sync())
		{			
			map<ThreadId, pair<ProcessPtr,ProcessPtr> >::iterator it;
			if ((it = queues.find (csp::CurrentThreadId())) == queues.end())
			{
				ProcessPtr process( currentProcess() );
				queues[csp::CurrentThreadId()] = make_pair(process,process);
			}
			else
			{
				addProcessToQueueAtHead(&(it->second.first),&(it->second.second),static_cast<ProcessPtr>(currentProcess()));
			}			
			
			end.release();
			reschedule();
		}
		else
		{
			end.release();
		}		
	}


	Mobile<BarrierEnd> end()
	{
		return Mobile<BarrierEnd>(new BarrierEnd(this,&queues));
	}
	
	Mobile<BarrierEnd> enrolledEnd()
	{		
		enroll();
		BarrierBase* base = this;
		return Mobile<BarrierEnd>(new BarrierEnd(base,base));
	}
};

		class BucketJoiner : public CSProcess
		{
		private:
			Bucket* bucket;
			
		protected:
			void run()
			{
				bucket->fallInto();
			}
		public:
			inline BucketJoiner(Bucket* _bucket)
				:	bucket(_bucket)
			{
			}
		};


class BarrierTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;

	std::list<TestResult (*) ()> tests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>
			(test0)(test1)(test2)(test3)
			(testBucket0)(testBucket1)(testBucket2)
		;
	}

	std::list<TestResult (*) ()> perfTests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>
			(oneThreadPerfTest100)(oneThreadPerfTest1000)(oneThreadPerfTest10000)(multiThreadPerfTest2)(multiThreadPerfTest2_5000)(multiThreadPerfTest10)(multiThreadPerfTest100) (multiThreadPerfTest100_100)
			(simple_oneThreadPerfTest100)(simple_oneThreadPerfTest1000)(simple_oneThreadPerfTest10000)(simple_multiThreadPerfTest2)(simple_multiThreadPerfTest2_5000)(simple_multiThreadPerfTest10)(simple_multiThreadPerfTest100)	(simple_multiThreadPerfTest100_100)
		;
	}


	//Just us syncing:
	static TestResult test0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		Barrier barrier;
								
		Mobile<BarrierEnd> end(barrier.end());		
				
		//Test 0:
			end->enroll();
			end->sync();
			end->resign();			
		//End test 0
		
		ASSERTL(events.empty(),"Single-sync blocked",__LINE__);

		END_TEST("Barrier Test 0");
	}

	//One other person syncing:
	static TestResult test1()
	{
		BEGIN_TEST()
	
		SetUp setup;
		Barrier barrier;
								
		Mobile<BarrierEnd> endUs(barrier.end());
	
		CSProcessPtr _syncer = new BarrierSyncer(barrier.end());
		ProcessPtr syncer(getProcessPtr(_syncer));		
		
		ScopedForking forking;
		
		endUs->enroll();
		
		EventList expA = tuple_list_of
			(us,syncer,syncer) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(syncer,NullProcessPtr,NullProcessPtr) //they must block
		;
		
		EventList actA;
		
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_syncer);
		
			CPPCSP_Yield();
		}
		
		EventList expB = tuple_list_of
			(us,syncer,syncer) //We complete the barrier, and free them, but don't context switch
		;
		
		EventList actB;
		{
			RecordEvents _(&actB);

			endUs->sync();
		}		
		
		EventList expC = tuple_list_of
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
		;

		EventList actC;
		{
			RecordEvents _(&actC);
		
			CPPCSP_Yield();
		}

		_syncer = new BarrierSyncer(barrier.end());
		syncer = getProcessPtr(_syncer);		

		EventList expD = tuple_list_of
			(us,syncer,syncer) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(syncer,NullProcessPtr,NullProcessPtr) //they must block
		;
		
		EventList actD;
		{
			RecordEvents _(&actD);

			forking.forkInThisThread(_syncer);
		
			CPPCSP_Yield();
		}		

		EventList expE = tuple_list_of
			(us,syncer,syncer) //We free them by resigning
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //they finish
		;
		
		EventList actE;
		{
			RecordEvents _(&actE);

			endUs->resign();
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Test 1 part A not as expected",__LINE__);
		ASSERTEQ(expB,actB,"Test 1 part B not as expected",__LINE__);
		ASSERTEQ(expC,actC,"Test 1 part C not as expected",__LINE__);
		ASSERTEQ(expD,actD,"Test 1 part D not as expected",__LINE__);
		ASSERTEQ(expE,actE,"Test 1 part E not as expected",__LINE__);

		END_TEST("Barrier Test 1");	
	}

	//One other person syncing, using half-enrollment:
	static TestResult test2()
	{
		BEGIN_TEST()
	
		SetUp setup;
		Barrier barrier;
								
		Mobile<BarrierEnd> endUs(barrier.enrolledEnd());
	
		//They will enroll(), we will not:
		CSProcessPtr _syncer = new BarrierSyncer(barrier.enrolledEnd());
		ProcessPtr syncer(getProcessPtr(_syncer));		
		
		ScopedForking forking;
		
		EventList expA = tuple_list_of
			(us,syncer,syncer) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(syncer,NullProcessPtr,NullProcessPtr) //they must block
		;
		
		EventList actA;

		{
			RecordEvents _(&actA);

			forking.forkInThisThread(_syncer);
		
			CPPCSP_Yield();
		}		
		
		EventList expB = tuple_list_of
			(us,syncer,syncer) //We complete the barrier, and free them, but don't context switch
		;

		EventList actB;
		{
			RecordEvents _(&actB);		

			endUs->sync();
		}		

		EventList expC = tuple_list_of
			(us,us,us)(us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
		;
		
		EventList actC;
		{
			RecordEvents _(&actC);		

			CPPCSP_Yield();
		}
		
		EventList expC2; //Nothing should happen, scheduling-wise
		EventList actC2;
		{
			RecordEvents _(&actC2);
			
			Mobile<BarrierEnd> endUs2(barrier.enrolledEnd());
			endUs2->resign();
		}
		
		_syncer = new BarrierSyncer(barrier.enrolledEnd());
		syncer = getProcessPtr(_syncer);		

		EventList expD = tuple_list_of
			(us,syncer,syncer) //We start them up
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(syncer,NullProcessPtr,NullProcessPtr) //they must block
		;
		

		EventList actD;

		{
			RecordEvents _(&actD);		

			forking.forkInThisThread(_syncer);
		
			CPPCSP_Yield();
		}		
		
		EventList expD2; //Nothing should happen, scheduling-wise
		EventList actD2;

		{
			RecordEvents _(&actD2);

			Mobile<BarrierEnd> endUs3(barrier.enrolledEnd());
			endUs3->resign();
			endUs3->resign(); //should do nothing
			Mobile<BarrierEnd> endUs4(barrier.enrolledEnd());
			endUs4->enroll();
			endUs4->resign();			
		}
				
		EventList expE = tuple_list_of
			(us,syncer,syncer) //We free them by resigning
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //we yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //they finish
		;
		

		EventList actE;
		{
			RecordEvents _(&actE);

			endUs->resign();
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Test 2 part A not as expected",__LINE__);
		ASSERTEQ(expB,actB,"Test 2 part B not as expected",__LINE__);
		ASSERTEQ(expC,actC,"Test 2 part C not as expected",__LINE__);
		ASSERTEQ(expC2,actC2,"Test 2 part C2 not as expected",__LINE__);
		ASSERTEQ(expD,actD,"Test 2 part D not as expected",__LINE__);
		ASSERTEQ(expD,actD,"Test 2 part D2 not as expected",__LINE__);
		ASSERTEQ(expE,actE,"Test 2 part E not as expected",__LINE__);

		END_TEST("Barrier Test 2");	
	}
	
	static map<csp::ThreadId,EventList::const_iterator> assertThreadedBarrierSync(const ThreadedEventList& eventList, map<csp::ThreadId,EventList::const_iterator>* iterators = NULL)
	{
		map<csp::ThreadId,EventList::const_iterator> ret;
		if (iterators == NULL)
		{
			for (ThreadedEventList::const_iterator it = eventList.begin();it != eventList.end();it++)
			{
				ret[it->first] = it->second.begin();				
			}
		}
		else
		{
			ret = *iterators;
		}
		
		//A barrier sync will consist of everyone blocking, and one person freeing all the other threads
		
		bool hadSyncer = false;
		set<ProcessPtr> blocked,freed;
		
		for (ThreadedEventList::const_iterator it = eventList.begin();it != eventList.end();it++)
		{
			//Note that this is a reference!
			EventList::const_iterator& jt = ret[it->first];
		
			if (jt == it->second.end())
			{
				ASSERTL(false,"No more events while looking for barrier sync",__LINE__);
			}
			else if (jt->get<0>() == NullProcessPtr)
			{
				ASSERTL(false,"Process finished while looking for barrier sync",__LINE__);
			}
			else if (jt->get<1>() == NullProcessPtr)
			{
				//Fine, they blocked
				blocked.insert(jt->get<0>());
				jt++;
			}
			else
			{
				ASSERTL(false == hadSyncer,"More than one process completed the sync",__LINE__);
				hadSyncer = true;
				while (jt != it->second.end() && jt->get<0>() != NullProcessPtr && jt->get<1>() != NullProcessPtr)
				{
					freed.insert(jt->get<1>());
					freed.insert(jt->get<2>()); //just in case they added two at once
					jt++;
				}
			}			
		}
		
		ASSERTL(hadSyncer,"No-one completed the sync",__LINE__);
		ASSERTEQ(blocked,freed,"Processes that blocked did not match processes that were freed",__LINE__);
		
		return ret;
	}

	//One other person syncing, in another thread:
	static TestResult test3()
	{
		BEGIN_TEST()
	
		SetUp setup;
		Barrier barrier;
								
		Mobile<BarrierEnd> endUs(barrier.end());
	
		CSProcessPtr _syncer = new BarrierSyncer(barrier.enrolledEnd());
//Unused:
//		ProcessPtr syncer(getProcessPtr(_syncer));		
				
		endUs->enroll();
		
		ThreadedEventList actA;
		
		{
			RecordEvents _(&actA);
			
			{
				ScopedForking forking;			
				
				forking.fork(_syncer);
		
				endUs->sync();
			}
		}
		
		endUs->resign();

		//Only check for the explicit sync:		
		assertThreadedBarrierSync(actA,NULL);

		END_TEST("Barrier Test 3");
	}

	template <typename BARRIER>	
	static TestResult _barrierPerfTest(const char* name,int numThreads, int numProcessesPerThread,int numSyncs)
	{
		Time start,finish;
		usign32 calls;
		double microsPerSync;
	
		BEGIN_TEST()
		
		BARRIER barrier;
		
		list< list<CSProcessPtr> > syncers;
		list<CSProcessPtr> specialSyncers;
		
		for (int i = 0;i < (numThreads - 1);i++)
		{
			list<CSProcessPtr> subSyncers;
			
			for (int j = 0;j < numProcessesPerThread;j++)
			{
				subSyncers.push_back(new BarrierSyncer(barrier.enrolledEnd(),numSyncs + 3));
			}
			
			syncers.push_back(subSyncers);
		}
		
		for (int j = 0;j < numProcessesPerThread - 1;j++)
		{
			specialSyncers.push_back(new BarrierSyncer(barrier.enrolledEnd(),numSyncs + 3));
		}
		
		Mobile<BarrierEnd> endUs(barrier.enrolledEnd());
		
		endUs->enroll();
		
		{
			ScopedForking forking;
			
			for (list< list<CSProcessPtr> >::iterator it = syncers.begin();it != syncers.end();it++)
			{
				forking.fork(InParallelOneThread(it->begin(),it->end()).process());
			}
			
			forking.forkInThisThread(specialSyncers.begin(),specialSyncers.end());
		
			//Two for warm-up:
			
			endUs->sync();

			CPPCSP_Yield();			
			CPPCSP_Yield();						
			CPPCSP_Yield();
			
			csp::usign32 volatile * wfp = &AtomicProcessQueue::waitFP_calls;
			
			AtomicPut(wfp,0);
			
			//This sync will free everyone, but they won't run yet (in single-threaded mode at least):
			endUs->sync();
		
			CurrentTime(&start);
			
			for (int i = 0;i < numSyncs;i++)
			{
				endUs->sync();
			}
			
			CurrentTime(&finish);
			
			calls = AtomicGet(wfp);
			
			finish -= start;
			
			microsPerSync = (GetSeconds(&finish) / static_cast<double>(numSyncs)) * 1000000.0;
			
			endUs->resign();			
			//Now everyone else does the last sync and finishes:
		}
		
		END_TEST(string("Barrier Performance Test(") + name + "), " + lexical_cast<string>(numThreads) + " threads, each with: " + lexical_cast<string>(numProcessesPerThread) + " user-level processes: " + lexical_cast<string>(microsPerSync) + " microseconds per sync, waits: " + lexical_cast<string>(calls));
	}


	static TestResult oneThreadPerfTest100()
	{
		return _barrierPerfTest<Barrier>("Barrier",1,100,10000);
	}
	
	static TestResult oneThreadPerfTest1000()
	{
		return _barrierPerfTest<Barrier>("Barrier",1,1000,1000);
	}
	
	static TestResult oneThreadPerfTest10000()
	{
		return _barrierPerfTest<Barrier>("Barrier",1,10000,100);
	}		
	
	static TestResult multiThreadPerfTest2()
	{
		return _barrierPerfTest<Barrier>("Barrier",2,1,100000);
	}
	
	static TestResult multiThreadPerfTest2_5000()
	{
		return _barrierPerfTest<Barrier>("Barrier",2,5000,100);
	}
	

	static TestResult multiThreadPerfTest10()
	{
		return _barrierPerfTest<Barrier>("Barrier",10,1,10000);
	}

	static TestResult multiThreadPerfTest100()
	{
		return _barrierPerfTest<Barrier>("Barrier",100,1,1000);
	}
	
	static TestResult multiThreadPerfTest100_100()
	{
		return _barrierPerfTest<Barrier>("Barrier",100,100,100);
	}
	


	static TestResult simple_oneThreadPerfTest100()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",1,100,10000);
	}
	
	static TestResult simple_oneThreadPerfTest1000()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",1,1000,1000);
	}
	
	static TestResult simple_oneThreadPerfTest10000()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",1,10000,100);
	}		
	
	static TestResult simple_multiThreadPerfTest2()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",2,1,100000);
	}
	
	static TestResult simple_multiThreadPerfTest2_5000()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",2,5000,100);
	}

	static TestResult simple_multiThreadPerfTest10()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",10,1,10000);
	}

	static TestResult simple_multiThreadPerfTest100()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",100,1,1000);
	}	
	
	static TestResult simple_multiThreadPerfTest100_100()
	{
		return _barrierPerfTest<SimpleBarrier<PureSpinMutex> >("Simple Pure-Spin Barrier",100,100,100);
	}
	
	
	//TODO later test multi-threading
	
	
	static TestResult testBucket0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		Bucket bucket;
								
		ASSERTL(bucket.processes.empty(),"Bucket not empty",__LINE__);
		ASSERTEQ(0,bucket.processCount,"Bucket not empty",__LINE__);
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket not empty",__LINE__);
		
		EventList expA; //empty
		
		EventList actA;
		{
			RecordEvents _(&actA);
		
			bucket.flush();
		}
		
		ASSERTEQ(expA,actA,"Emptying an empty bucket did something",__LINE__);
		ASSERTL(bucket.processes.empty(),"Bucket not empty",__LINE__);
		ASSERTEQ(0,bucket.processCount,"Bucket not empty",__LINE__);
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket not empty",__LINE__);							

		END_TEST("Bucket Test 0");
	}
	
	static TestResult testBucket1()
	{
		Bucket bucket;
		ScopedForking forking;
	
		BEGIN_TEST()
		
		SetUp setup;		
		
		CSProcessPtr _joiner = new BucketJoiner(&bucket);
		ProcessPtr joiner(getProcessPtr(_joiner));		
				
		EventList expA = tuple_list_of
			(us,joiner,joiner) //We fork them		
			(us,us,us)(us,NullProcessPtr,NullProcessPtr) //We yield
			(joiner,NullProcessPtr,NullProcessPtr) //They block on the bucket			
		;
		
		EventList actA;
		{
			RecordEvents _(&actA);
		
			forking.forkInThisThread(_joiner);
			
			CPPCSP_Yield();
		}
		
		std::map<ThreadId,std::pair<ProcessPtr,ProcessPtr> > expProcessesA;
		expProcessesA[CurrentThreadId()] = make_pair(joiner,joiner);
		
		ASSERTEQ(expA,actA,"Events not as expected",__LINE__);
		ASSERTEQ(expProcessesA,bucket.processes,"Bucket processes not as expected",__LINE__);
		ASSERTEQ(1,bucket.processCount,"Bucket count not as expected",__LINE__);
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket mutex is claimed",__LINE__);
		
		CSProcessPtr _joiner2 = new BucketJoiner(&bucket);
		ProcessPtr joiner2(getProcessPtr(_joiner2));		
				
		EventList expB = tuple_list_of
			(us,joiner2,joiner2) //We fork them		
			(us,us,us)(us,NullProcessPtr,NullProcessPtr) //We yield
			(joiner2,NullProcessPtr,NullProcessPtr) //They block on the bucket			
		;
		
		EventList actB;
		{
			RecordEvents _(&actB);
		
			forking.forkInThisThread(_joiner2);
			
			CPPCSP_Yield();
		}
		
		std::map<ThreadId,std::pair<ProcessPtr,ProcessPtr> > expProcessesB;
		expProcessesB[CurrentThreadId()] = make_pair(joiner,joiner2);
		
		ASSERTEQ(expB,actB,"Events not as expected",__LINE__);
		ASSERTEQ(expProcessesB,bucket.processes,"Bucket processes not as expected",__LINE__);
		ASSERTEQ(2,bucket.processCount,"Bucket count not as expected",__LINE__);
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket mutex is claimed",__LINE__);


		EventList expC = tuple_list_of
			(us,joiner,joiner2) //We flush them			
		;
		
		EventList actC;
		{
			RecordEvents _(&actC);
		
			bucket.flush();
		}
		
		std::map<ThreadId,std::pair<ProcessPtr,ProcessPtr> > expProcessesC;		
		
		ASSERTEQ(expC,actC,"Events not as expected",__LINE__);
		ASSERTEQ(expProcessesC,bucket.processes,"Bucket processes not as expected",__LINE__);
		ASSERTEQ(0,bucket.processCount,"Bucket count not as expected",__LINE__);
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket mutex is claimed",__LINE__);


		END_TEST_C("Bucket Test 1",bucket.flush(););
	}
	
	template <typename T,typename K>
	static std::set<T> values(const std::map<K,T>& data)
	{
		std::set<T> r;
		for (typename std::map<K,T>::const_iterator it = data.begin();it != data.end();it++)
		{
			r.insert(it->second);
		}
		return r;
	}

	//test multi-threaded buckets:
	
	static TestResult testBucket2()
	{
		Bucket bucket;
		ScopedForking forking;
	
		BEGIN_TEST()
		
		SetUp setup;		
		
		CSProcessPtr _joiner0 = new BucketJoiner(&bucket);
		ProcessPtr joiner0(getProcessPtr(_joiner0));
		CSProcessPtr _joiner1 = new BucketJoiner(&bucket);
		ProcessPtr joiner1(getProcessPtr(_joiner1));
		CSProcessPtr _joiner2 = new BucketJoiner(&bucket);
		ProcessPtr joiner2(getProcessPtr(_joiner2));
		CSProcessPtr _joiner3 = new BucketJoiner(&bucket);
		ProcessPtr joiner3(getProcessPtr(_joiner3));
				
		forking.forkInThisThread(_joiner0);
		forking.fork(InParallelOneThread(_joiner1)(_joiner2));
		forking.fork(_joiner3);
			
		while (bucket.holding() < 4)
		{
			Thread_Yield();
			CPPCSP_Yield();
		}
		
		std::set<std::pair<ProcessPtr,ProcessPtr> > expProcessesA,expProcessesB;
		expProcessesA.insert(make_pair(joiner0,joiner0));
		expProcessesA.insert(make_pair(joiner1,joiner2));
		expProcessesA.insert(make_pair(joiner3,joiner3));
		
		expProcessesB.insert(make_pair(joiner0,joiner0));
		expProcessesB.insert(make_pair(joiner2,joiner1));
		expProcessesB.insert(make_pair(joiner3,joiner3));
		
		ASSERTEQ1OF2(expProcessesA,expProcessesB,values(bucket.processes),"Bucket processes not as expected",__LINE__);		
		ASSERTEQ(false,bucket.mutex.isClaimed(),"Bucket mutex is claimed",__LINE__);
		
		bucket.flush();

		END_TEST_C("Bucket Test 2",bucket.flush(););
	}
	
	inline virtual ~BarrierTest()
	{
	}
};

ProcessPtr BarrierTest::us;



Test* GetBarrierTest()
{
	return new BarrierTest;
}
