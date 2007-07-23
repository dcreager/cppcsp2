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

template <typename BUF_CHANNEL>
	class FIFO1 : public BUF_CHANNEL
	{
	public:
		inline FIFO1()
			:	BUF_CHANNEL(FIFOBuffer<int>::Factory(1))
		{
		}
	};
	
	template <typename T> class ChannelName<FIFO1<BufferedOne2OneChannel<T> > > {public:static string Name() {return "BufferedOne2OneChannel";}};
	template <typename T> class ChannelName<FIFO1<BufferedOne2AnyChannel<T> > > {public:static string Name() {return "BufferedOne2AnyChannel";}};
	template <typename T> class ChannelName<FIFO1<BufferedAny2OneChannel<T> > > {public:static string Name() {return "BufferedAny2OneChannel";}};
	template <typename T> class ChannelName<FIFO1<BufferedAny2AnyChannel<T> > > {public:static string Name() {return "BufferedAny2AnyChannel";}};	

//namespace
//{

	template <typename DATA_TYPE, typename CONDITION>
	class ConditionOne2OneChannel : private internal::BaseChan<DATA_TYPE>, private virtual internal::PoisonableChan
	{
	private:			
		union
		{
			volatile const DATA_TYPE* src;
			volatile DATA_TYPE* dest;
		};
		
		CONDITION cond;
	
		void checkPoison()
		{
			if (isPoisoned)
				throw PoisonException();
		}
		
		#define WRITER 0
		#define READER 1

		void input(DATA_TYPE* const _dest)
		{
			checkPoison();
		
			cond.claim();
			
			if (isPoisoned)
			{
				cond.release();
				throw PoisonException();
			}
			
			if (src != NULL)
			{
				//Writer waiting:
				*_dest = *src;
				src = NULL;
				cond.signal(WRITER);
				cond.release();				
			}
			else
			{
				//No-one waiting:
				dest = _dest;								
				cond.releaseWait(NULL,READER);				
				//Now communication has finished (or we've been poisoned)
				checkPoison();
				
			}
		}
		
		//Extended input is unneeded for commstime so don't bother:
		void beginExtInput(DATA_TYPE* const)
		{			
		}
		
		void endExtInput()
		{			
		}
		
		void output(const DATA_TYPE* const _src)
		{
			checkPoison();
					
			cond.claim();
			
			if (isPoisoned)
			{
				cond.release();
				throw PoisonException();
			}
			
			if (dest != NULL)
			{				
				//Reader waiting:
				*dest = *_src;
				dest = NULL;
				cond.signal(READER);
				cond.release();
			}
			else
			{
				//No-one waiting:
				src = _src;				
				cond.releaseWait(NULL,WRITER);				
				//Now communication has finished (or we've been poisoned)
				checkPoison();
			}		
		}
		
		void poisonIn()
		{	
			cond.claim();
			
			isPoisoned = true;
			
			cond.signal(READER);
			cond.signal(WRITER);
			
			cond.release();
		}
		
		void poisonOut()
		{
			poisonIn();
		}
			
	public:
		inline ConditionOne2OneChannel()			
		{
			//Do both, just to be sure:
			src = NULL;
			dest = NULL;
		}
	
		Chanin<DATA_TYPE> reader()
		{
			return Chanin<DATA_TYPE>(this,true);
		}
		
		Chanout<DATA_TYPE> writer()
		{
			return Chanout<DATA_TYPE>(this,true);
		}
		
		#undef WRITER
		#undef READER
							
		//For testing:
		friend class ChannelTest;				
	};


template<unsigned ITERATIONS = 10000>
class _Recorder : public CSProcess
{
private:
	Chanin<int> in;
	Chanout<Time> out;
protected:
	void run()
	{
		int n = 0;
		
		for (unsigned i = 0;i < (ITERATIONS / 100);i++)
		{
			in >> n;
		}
		
		Time start,finish;
		
		CurrentTime(&start);	
		for (unsigned i = 0;i < ITERATIONS;i++)
		{
			in >> n;
		}
		CurrentTime(&finish);
		
		out << (finish - start);
				
		in.poison();
	}
public:
	inline _Recorder(const Chanin<int>& _in, const Chanout<Time>& _out)
		:	CSProcess(65536),in(_in),out(_out)
	{		
	}
};

typedef _Recorder<> Recorder;

class RecorderId : public CSProcess
{
private:
	Chanin<int> in;
	Chanout<int> out;
	Chanout<Time> outTime;
protected:
	void run()
	{
		int n = 0;
		
		for (int i = 0;i < 100;i++)
		{
			in >> n;
			out << n;
		}
		
		Time start,finish;
		
		CurrentTime(&start);	
		for (int i = 0;i < 10000;i++)
		{
			in >> n;
			out << n;
		}
		CurrentTime(&finish);
		
		outTime << (finish - start);
				
		in.poison();
		out.poison();
	}
public:
	inline RecorderId(const Chanin<int>& _in, const Chanout<int>& _out, const Chanout<Time>& _outTime)
		:	CSProcess(65536),in(_in),out(_out),outTime(_outTime)
	{		
	}
};

template <typename DATA_TYPE>
	class SeqAdder : public CSProcess
	{
	private:
		DATA_TYPE t0,t1;
		Chanin<DATA_TYPE> in0;
		Chanin<DATA_TYPE> in1;
		Chanout<DATA_TYPE> out;
	protected:
		void run()
		{			
			try
			{
				while (true)
				{
					in0 >> t0;
					in1 >> t1;				
					out << (t0 + t1);
				}
			}
			catch (PoisonException&)
			{
				in0.poison();
				in1.poison();
				out.poison();
			}
		}
	public:
		inline SeqAdder(const Chanin<DATA_TYPE>& _in0, const Chanin<DATA_TYPE>& _in1, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in0(_in0),in1(_in1),out(_out)
		{
		}
	};


	template <typename DATA_TYPE>
	class ExtReader : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;		
		DATA_TYPE t;		
	protected:
		void run()
		{			
			try
			{
				//Just once:
				{ 
					ScopedExtInput<DATA_TYPE> extInput(in,&t);
				}
				
			}
			catch (PoisonException&)
			{
				in.poison();		
			}		
		}
	public:		
		inline ExtReader(const Chanin<DATA_TYPE>& _in)
			:	CSProcess(65536),in(_in)
		{
		}		
		
		#ifndef CPPCSP_DOXYGEN
		//For testing:
		friend class ::BufferedChannelTest;
		#endif
	};


class ChannelTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;
	
	static void checkReadMutex(One2OneChannel<int>&,bool,int) {}
	static void checkReadMutex(One2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.readerMutex.isClaimed(),"Read mutex (is/not) claimed",line);}
	static void checkReadMutex(Any2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.readerMutex.isClaimed(),"Read mutex (is/not) claimed",line);}
	static void checkWriteMutex(One2OneChannel<int>&,bool,int) {}
	static void checkWriteMutex(Any2OneChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.writerMutex.isClaimed(),"Write mutex (is/not) claimed",line);}
	static void checkWriteMutex(Any2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.writerMutex.isClaimed(),"Write mutex (is/not) claimed",line);}
	
	template <typename CHANNEL>
	inline static void checkReadWriteMutex(CHANNEL& c,bool readClaimed,bool writeClaimed,int line)
	{
		checkReadMutex(c,readClaimed,line);
		checkWriteMutex(c,writeClaimed,line);
	}

	template <typename CHANNEL>
	static TestResult _commsTimeTest(const char* name, bool useThreads)
	{
		Time time;
	
		BEGIN_TEST()		
		
		CHANNEL a,b,c,d;
		BufferedOne2OneChannel<Time> result(FIFOBuffer<Time>::Factory(1));
		
		CSProcessPtr prefix = new Prefix<int>(a.reader(),b.writer(),0);
		CSProcessPtr delta = new SeqDelta<int>(b.reader(),c.writer(),d.writer());
		CSProcessPtr succ = new Successor<int>(c.reader(),a.writer());
		CSProcessPtr rec = new Recorder(d.reader(),result.writer());
		
		if (false == useThreads)
		{
			Run(InParallelOneThread
				(prefix)
				(delta)
				(succ)
				(rec)
			);
		}
		else
		{
			Run(InParallel
				(prefix)
				(delta)
				(succ)
				(rec)
			);
		}
		
		result.reader() >> time;

		END_TEST(string("CommsTime Test (") + name + "): " + lexical_cast<string>(GetSeconds(&time) * 100.0) + " microseconds");
	}
	
	template <typename CHANNEL>
	static TestResult _triangleCommsTimeTest(const char* name, int kernelThreads /*1,2 or "other"*/)
	{
		Time time;
	
		BEGIN_TEST()
		
		CHANNEL a,b,c,d,e,f,g;
		BufferedOne2OneChannel<Time> result(FIFOBuffer<Time>::Factory(1));
		
		CSProcessPtr prefixA = new Prefix<int>(a.reader(),b.writer(),0);		
		CSProcessPtr delta = new SeqDelta<int>(b.reader(),c.writer(),d.writer());
		CSProcessPtr succ = new Successor<int>(c.reader(),a.writer());
		
		CSProcessPtr adder = new SeqAdder<int>(d.reader(),g.reader(),e.writer());
		CSProcessPtr recId = new RecorderId(e.reader(),f.writer(),result.writer());		
		CSProcessPtr prefixB = new Prefix<int>(f.reader(),g.writer(),0);		
		
		switch (kernelThreads)
		{
			case 1:
				Run(InParallelOneThread
					(prefixA)
					(delta)
					(succ)
					(adder)
					(recId)
					(prefixB)
				);
				break;
			case 2:
				Run(InParallel
					(InParallelOneThread
						(prefixA)
						(delta)
						(succ)
					)
					(InParallelOneThread
						(adder)
						(recId)
						(prefixB)
					)
				);
				break;
			default:
				Run(InParallel
					(prefixA)
					(delta)
					(succ)
					(adder)
					(recId)
					(prefixB)
				);
				break;
		}
		
		result.reader() >> time;

		END_TEST(string("Triangular CommsTime Test (") + name + "): " + lexical_cast<string>(GetSeconds(&time) * 100.0) + " microseconds");
	}
	
	template <typename CHANNEL>
	static TestResult _doublecommsTimeTest(const char* name, bool useThreads)
	{
		Time time;
	
		BEGIN_TEST()		
		
		CHANNEL a,b,c,d;
		BufferedOne2OneChannel<Time> result(FIFOBuffer<Time>::Factory(1));
		
		CSProcessPtr prefix = new Prefix<int>(a.reader(),b.writer(),0,2);		
		CSProcessPtr delta = new SeqDelta<int>(b.reader(),c.writer(),d.writer());
		CSProcessPtr succ = new Successor<int>(c.reader(),a.writer());
		CSProcessPtr rec = new Recorder(d.reader(),result.writer());
		
		if (false == useThreads)
		{
			Run(InParallelOneThread
				(prefix)				
				(delta)
				(succ)
				(rec)
			);
		}
		else
		{
			Run(InParallel
				(prefix)				
				(delta)
				(succ)
				(rec)
			);
		}
		
		result.reader() >> time;

		END_TEST(string("Double CommsTime Test (") + name + "): " + lexical_cast<string>(GetSeconds(&time) * 100.0) + " microseconds");
	}
	
	template <typename CHANNEL>
	static TestResult _pingPongTest(const char* name, bool useThreads)
	{
		Time time;
		
		csp::usign32 volatile * wfp = &AtomicProcessQueue::waitFP_calls;
	
		BEGIN_TEST()		
		
		CHANNEL a;
		BufferedOne2OneChannel<Time> result(FIFOBuffer<Time>::Factory(1));
		
		CSProcessPtr writer = new WriterProcess<int>(a.writer(),0,200000); //it will be poisoned, not finish normally
		CSProcessPtr rec = new _Recorder<100000>(a.reader(),result.writer());				
		
		AtomicPut(wfp,0);
		
		if (false == useThreads)
		{
			Run(InParallelOneThread
				(writer)
				(rec)
			);
		}
		else
		{
			Run(InParallel
				(writer)
				(rec)
			);
		}
		
		result.reader() >> time;

		END_TEST(string("Ping-Pong Test (") + name + "): " + lexical_cast<string>(GetSeconds(&time) * 10.0) + " microseconds, waits: " + lexical_cast<string>(AtomicGet(wfp)));
	}
	
	static TestResult pingPongTest0()
	{
		return _pingPongTest< One2OneChannel<int> >("User-Threads, Hybrid",false);
	}
	
	static TestResult pingPongTest1()
	{
		return _pingPongTest< _One2OneChannel<int,NullMutex> >("User-Threads, Plain",false);
	}
	
	static TestResult pingPongTest2()
	{
		return _pingPongTest< One2OneChannel<int> >("Threads, Hybrid",true);
	}
	
	/*
	static TestResult pingPongTest3()
	{
		return _pingPongTest< ConditionOne2OneChannel<int,MutexAndEvent<PureSpinMutex,2> > >("Threads, Plain",true);
	}
	*/
	
	static TestResult commsTimeTest0()
	{
		return _commsTimeTest< One2OneChannel<int> >("Normal",false);
	}
	
	static TestResult commsTimeTest1()
	{
		return _commsTimeTest< One2OneChannel<int> >("With Threads",true);
	}	
	
	static TestResult commsTimeTest0Double()
	{
		return _doublecommsTimeTest< One2OneChannel<int> >("Normal",false);
	}
	
	static TestResult commsTimeTest1Double()
	{
		return _doublecommsTimeTest< One2OneChannel<int> >("With Threads",true);
	}	
	
	static TestResult commsTimeTest0Triangle()
	{
		return _triangleCommsTimeTest< One2OneChannel<int> >("Single-Threaded",1);
	}
	
	static TestResult commsTimeTest1Triangle()
	{
		return _triangleCommsTimeTest< One2OneChannel<int> >("Two Threads",2);
	}
	
	static TestResult commsTimeTest2Triangle()
	{
		return _triangleCommsTimeTest< One2OneChannel<int> >("All Threads",6);
	}
	
	static TestResult commsTimeTest2()	
	{
		return _commsTimeTest< _One2OneChannel<int,NullMutex> >("Normal, No Mutex",false);
	}
	
	static TestResult commsTimeTest3()	
	{
		return _commsTimeTest< _One2OneChannel<int,PureSpinMutex> >("Normal, Pure-Spin Mutex",false);
	}
	
	static TestResult commsTimeTest4()
	{
		return _commsTimeTest< _One2OneChannel<int,PureSpinMutex> >("Threads, Pure-Spin Mutex",true);
	}
	
	static TestResult commsTimeTest5()	
	{
		return TestResultPass("CT5"); //_commsTimeTest< _One2OneChannel<int,QueuedMutex> >("Normal, Queued Mutex",false);
	}
	
	static TestResult commsTimeTest6()
	{
		return TestResultPass("CT6"); //_commsTimeTest< _One2OneChannel<int,QueuedMutex> >("Threads, Queued Mutex",true);
	}
	
	/*
	static TestResult commsTimeTest7()
	{
		return _commsTimeTest< ConditionOne2OneChannel<int, MutexAndEvent<OSBlockingMutex,2> > >("Threads, Native Mutex and Native Event",true);
	}
	
	static TestResult commsTimeTest8()
	{
		return _commsTimeTest< ConditionOne2OneChannel<int,MutexAndEvent<PureSpinMutex,2> > >("Threads, Pure-Spin Mutex and Native Event",true);
	}
	
	static TestResult commsTimeTest8B()
	{
		return _commsTimeTest< ConditionOne2OneChannel<int,Condition<2> > >("Threads, Condition",true);
	}
	*/
	
	static TestResult commsTimeTest9()	
	{
		return _commsTimeTest< _One2OneChannel<int,PureSpinMutex_TTS> >("Normal, Pure-Spin Mutex TTS",false);
	}
	
	static TestResult commsTimeTest10()
	{
		return _commsTimeTest< _One2OneChannel<int,PureSpinMutex_TTS> >("Threads, Pure-Spin Mutex TTS",true);
	}
	
	template <typename CHANNEL>
	static TestResult test0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                			
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
			
			EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We release them from the channel
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);

				int n;
				c.reader() >> n;				
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected (B)",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected (B)",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
			CPPCSP_Yield();
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Test Writer");
	}
	
	template <typename CHANNEL>
	static TestResult test1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		
		{
			ScopedForking forking;
			
	        ReaderProcess<int>* _reader = new ReaderProcess<int>(c.reader(),1);
    		ProcessPtr reader(getProcessPtr(_reader));
                			
			
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(reader,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);
			
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We release them from the channel
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				c.writer() << 0 ;				
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected (B)",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected (B)",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);
			CPPCSP_Yield();
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Test Reader");
	}
	
	template <typename CHANNEL>
	static TestResult testPoison0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c0,c1,c2,c3;
		
		{
			ScopedForking forking;
			
		//Writer:
	        WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
    		
    		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c0.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c0,false,false,__LINE__);
    		
    		c0.reader().poison();
    		
    		ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c0.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c0,false,false,__LINE__);

			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They find the poison, and finish
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
			
			ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c0.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c0,false,false,__LINE__);
			
			_writer = new WriterProcess<int>(c1.writer(),0,1);
    		writer = getProcessPtr(_writer);
    		
    		ASSERTEQ(false,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c1.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c1,false,false,__LINE__);

			EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
				(us,writer,writer) //We poison the channel
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They wake-up and finish 
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
				
				ASSERTEQ(false,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				ASSERTEQ(false,c1.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
				checkReadWriteMutex(c1,false,true,__LINE__);
				
				c1.reader().poison();
								
				ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);				
				ASSERTEQ(false,c1.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
				checkReadWriteMutex(c1,false,true,__LINE__);
				
				CPPCSP_Yield();								
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
			
			ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(One2OneChannel<int>,c1,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c1.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c1,false,false,__LINE__);
			
		//Reader:
			
			ReaderProcess<int>* _reader = new ReaderProcess<int>(c2.reader(),1);
    		ProcessPtr reader(getProcessPtr(_reader));
    		
    		ASSERTEQ(false,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c2.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c2,false,false,__LINE__);
    		
    		c2.writer().poison();
    		
    		ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c2.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c2,false,false,__LINE__);

			EventList expC;
			
			expC = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They find the poison, and finish
			;
			
			EventList actC;
			{
				RecordEvents _(&actC);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
			
			ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			ASSERTEQ(NullProcessPtr,c2.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c2,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,c2.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			
			_reader = new ReaderProcess<int>(c3.reader(),1);
    		reader = getProcessPtr(_reader);
    		
    		ASSERTEQ(false,c3.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
    		ASSERTEQ(false,c3.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
    		checkReadWriteMutex(c3,false,false,__LINE__);

			EventList expD;
			
			expD = tuple_list_of 
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the channel
				(us,reader,reader) //We poison the channel
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They wake-up and finish 
			;
			
			EventList actD;
			{
				RecordEvents _(&actD);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
				
				ASSERTEQ(false,c3.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				ASSERTEQ(false,c3.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
				checkReadWriteMutex(c3,true,false,__LINE__);
				
				c3.writer().poison();
								
				ASSERTEQ(true,c3.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);				
				ASSERTEQ(false,c3.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
				checkReadWriteMutex(c3,true,false,__LINE__);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expD,actD,"Channel events not as expected in part D",__LINE__);
			
			ASSERTEQ(true,c3.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			ASSERTEQ(NullProcessPtr,c3.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,c3,dest),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c3.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c3,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Test Poison");
	}			

	#ifdef CPPCSP_PARCOMM
	TestResult testParcomm()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		One2OneChannel<int> c0,c1;
		
		{
			ScopedForking forking;
			
			ParWriterProcess<int>* _writer = new ParWriterProcess<int>(list_of< Chanout<int> >(c0.writer(),c1.writer()),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
    		
    		EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
			ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected",__LINE__);
		}
	}
	#endif //CPPCSP_PARCOMM
	
	/**
	*	Tests an extended input with the reader arriving first
	*/
	template <typename CHANNEL>
	static TestResult testExtended0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		BlackHoleChannel<int> hole;
		Barrier barrier;
		Mobile<BarrierEnd> end(barrier.end());
		end->enroll();
		
		{
			ScopedForking forking;
			
	        ExtSyncId<int>* _id = new ExtSyncId<int>(c.reader(),hole.writer(),barrier.enrolledEnd());
    		ProcessPtr id(getProcessPtr(_id));
                			
			
			EventList expA;
			
			expA = tuple_list_of
				(us,id,id) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(id,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_id);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(id,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
			
			EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We fork the writer
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,id,id) //Writer re-schedules the id process to run
				(writer,NullProcessPtr,NullProcessPtr) //Writer blocks
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);

				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c,true,true,__LINE__);
			
			EventList expC;
			
			expC = tuple_list_of				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(id,NullProcessPtr,NullProcessPtr) //The id process blocks on the barrier				
			;
			
			EventList actC;
			{
				RecordEvents _(&actC);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c,true,true,__LINE__);
			
			EventList expD;
			
			expD = tuple_list_of				
				(us,id,id) //We sync on the barrier, releasing the id process
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				
				(id,writer,writer) //The id process finishes the extended rendezvous
				(id,NullProcessPtr,NullProcessPtr) //The id process blocks on the next input
				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield again
				
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
			;
			
			EventList actD;
			{
				RecordEvents _(&actD);
				
				end->sync();
				
				CPPCSP_Yield();
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expD,actD,"Channel events not as expected in part D",__LINE__);
						
			ASSERTEQ(id,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel data (mutex-claimed) not as expected",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);

			//Bit ugly, but safe:
			
			c.writer().poison();
			
			end->resign();
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Extended Test 0");
	}
	
	/**
	*	Tests an extended input with the writer arriving first
	*/
	template <typename CHANNEL>
	static TestResult testExtended1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		BlackHoleChannel<int> hole;
		Barrier barrier;
		Mobile<BarrierEnd> end(barrier.end());
		end->enroll();
		
		{
			ScopedForking forking;
			
	        ExtSyncId<int>* _id = new ExtSyncId<int>(c.reader(),hole.writer(),barrier.enrolledEnd());
    		ProcessPtr id(getProcessPtr(_id));
    		WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
    		
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
			
			EventList expB;
			
			expB = tuple_list_of
				(us,id,id) //We fork the id
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(id,NullProcessPtr,NullProcessPtr) //The id process blocks on the barrier
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);

				forking.forkInThisThread(_id);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);			
			checkReadWriteMutex(c,true,true,__LINE__);
			
			EventList expC;
			
			expC = tuple_list_of				
				(us,id,id) //We sync on the barrier, releasing the id process
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				
				(id,writer,writer) //The id process finishes the extended rendezvous
				(id,NullProcessPtr,NullProcessPtr) //The id process blocks on the next input
				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield again
				
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
			;
			
			EventList actC;
			{
				RecordEvents _(&actC);
				
				end->sync();
				
				CPPCSP_Yield();
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
						
			ASSERTEQ(id,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);


			//Bit ugly, but safe:
			
			c.writer().poison();
			
			end->resign();
			
		}
	
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Extended Test 1");
	}		
	
	/**
	*	Tests poisoning the channel during an extended input.  The end of an extended input should not
	*	then throw a poison exception
	*/
	template <typename CHANNEL>
	static TestResult testExtendedPoison()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;		
		
		{
			ScopedForking forking;
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
				        
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork the writer
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield				
				(writer,NullProcessPtr,NullProcessPtr) //Writer blocks on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);

				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
			
			EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We poison the channel, freeing the writer
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				try
				{
					{
						int n;
						ScopedExtInput<int> extInput(c.reader(),&n);
				
						c.reader().poison();
					}
				}
				catch (PoisonException)
				{
					ASSERTL(false,"Detected poison",__LINE__);
				}
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
			
			
			EventList expC;
			
			expC = tuple_list_of
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
			;
			
			EventList actC;
			{
				RecordEvents _(&actC);				
				
				CPPCSP_Yield();				
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Extended Poison Mid-way Test");
	}
	
	class DummyException {};
	
	/**
	*	Tests that an extended input is finished properly even if an exception is thrown that unwinds
	*	a ScopedExtInput.  The channel should not be poisoned in this case.	
	*/
	template <typename CHANNEL>
	static TestResult testExtException()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;		
		
		{
			ScopedForking forking;
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
    		
    		forking.forkInThisThread(_writer);
    		
    		//Get them blocked on the channel:
    		CPPCSP_Yield();
    		
    		ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c,false,true,__LINE__);
    		
    		EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We finish the extended input and free the writer
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
    		
    			try
	    		{
					{
						int n;
    					ScopedExtInput<int> extInput(c.reader(),&n);
    					
    					throw DummyException();
    				}
    			}
    			catch (DummyException)
    			{
    			}
    		}
    		
    		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
    		
    		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
    		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			checkReadWriteMutex(c,false,true,__LINE__);
			CPPCSP_Yield();
			checkReadWriteMutex(c,false,false,__LINE__);
			
    	}
    	
    	END_TEST(ChannelName<CHANNEL>::Name() + " Unbuffered Extended Exception Mid-way Test");
	}
	
	/**
	*	Tests shared writing of any2* channels
	*/
	template <typename CHANNEL>
	static TestResult testAny20()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
               
            ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel mutex claimed",__LINE__);
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer->t),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			
			WriterProcess<int>* _writer1 = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer1(getProcessPtr(_writer1));
			
			EventList expB;
			
			expB = tuple_list_of
				(us,writer1,writer1) //We fork another writer
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer1,NullProcessPtr,NullProcessPtr) //They block on the mutex
			;			
			
			EventList actB;
			{
				RecordEvents _(&actB);

				forking.forkInThisThread(_writer1);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
			
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer->t),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			
			
			EventList expC;
			
			expC = tuple_list_of
				(us,writer,writer) //We read from the channel and release the writer
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,writer1,writer1) //They release the other writer from the mutex
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
			;			
			
			EventList actC;
			{
				RecordEvents _(&actC);

				int n;
				c.reader() >> n;
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part B",__LINE__);
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			
			EventList expD;
			
			expD = tuple_list_of				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer1,NullProcessPtr,NullProcessPtr) //The second writer blocks on the channel				
			;			
			
			EventList actD;
			{
				RecordEvents _(&actD);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expD,actD,"Channel events not as expected in part B",__LINE__);
			ASSERTEQ(writer1,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer1->t),ACCESS(One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel mutex not claimed",__LINE__);

			//Free the writer so it can finish			
			int n;
			c.reader() >> n;
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Any2* Unbuffered Test 0");
	}
	
	/**
	*	Tests shared normal-reading of *2any channels
	*/
	template <typename CHANNEL>
	static TestResult test2Any0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c;
		
		{
			ScopedForking forking;
			
	        ReaderProcess<int>* _reader = new ReaderProcess<int>(c.reader(),1);
    		ProcessPtr reader(getProcessPtr(_reader));
            
            ASSERTEQ(false,c.readerMutex.isClaimed(),"Shared mutex claimed",__LINE__);    			
			
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(reader,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,c.readerMutex.isClaimed(),"Shared mutex not claimed",__LINE__);
			
			ReaderProcess<int>* _reader1 = new ReaderProcess<int>(c.reader(),1);
    		ProcessPtr reader1(getProcessPtr(_reader1));
			
			EventList expB;
			
			expB = tuple_list_of
				(us,reader1,reader1) //We fork another reader
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader1,NullProcessPtr,NullProcessPtr) //They block on the mutex
			;			
			
			EventList actB;
			{
				RecordEvents _(&actB);

				forking.forkInThisThread(_reader1);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
			
			ASSERTEQ(reader,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,c.readerMutex.isClaimed(),"Shared mutex not claimed",__LINE__);
			
			
			EventList expC;
			
			expC = tuple_list_of
				(us,reader,reader) //We read from the channel and release the reader
				
			;			
			
			EventList actC;
			{
				RecordEvents _(&actC);
				
				c.writer() << 8;							
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part B",__LINE__);
			ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);			
			ASSERTEQ(8,_reader->t,"Channel data was not read correctly",__LINE__);
			ASSERTEQ(true,c.readerMutex.isClaimed(),"Shared mutex not claimed",__LINE__);
			
			EventList expD;
			
			expD = tuple_list_of				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,reader1,reader1) //They release the other reader from the mutex
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader1,NullProcessPtr,NullProcessPtr) //The second reader blocks on the channel				
			;			
			
			EventList actD;
			{
				RecordEvents _(&actD);
				
				CPPCSP_Yield();
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expD,actD,"Channel events not as expected in part B",__LINE__);
			ASSERTEQ(reader1,c.waiting,"Channel data (waiting) not as expected",__LINE__);						
			ASSERTEQ(&(_reader1->t),ACCESS(One2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,c.readerMutex.isClaimed(),"Shared mutex not claimed",__LINE__);

			//Free the reader so it can finish			
			c.writer () << 0;
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " *2Any Unbuffered Test 0");
	}
	
	/**
	*	Tests extended input on Any2* channels
	*/
	template <typename CHANNEL>
	static TestResult testExtAny20()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c,d;
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
               
            ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel shared mutex claimed",__LINE__);
            ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex claimed",__LINE__);
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			
			
			{
				{
					int n;
					ScopedExtInput<int> extInput(c.reader(),&n);
					
					ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
					ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
					ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
					ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(true,c.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				
				CPPCSP_Yield();
				
				ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			}
			
			//now let the reader arrive first:
			
			_writer = new WriterProcess<int>(d.writer(),0,1);
    		writer = getProcessPtr(_writer);
    		
    		EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We fork them
				(us,NullProcessPtr,NullProcessPtr) //We block on the channel
				(writer,us,us) //They arrive and free us
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
				
				(us,writer,writer) //We end the extended input and free them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
    			forking.forkInThisThread(_writer);
    			
    			ASSERTEQ(false,d.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
    			{
    				int n;
    				ScopedExtInput<int> extInput(d.reader(),&n);
    				//we will block, but then the writer will run, block, and free us
    				
    				ASSERTEQ(writer,d.waiting,"Channel data (waiting) not as expected",__LINE__);			
					ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
					ASSERTEQ(true,d.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
					ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(true,d.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				
				CPPCSP_Yield();
				
				ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(false,d.writerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			}						
		
			ASSERTEQ(expB,actB,"Events not as expected",__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Any2* Unbuffered Extended Test 0");
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////
	
	/**
	*	Tests extended input on *2Any channels
	*/
	template <typename CHANNEL>
	static TestResult testExt2Any0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c,d;
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,1);
    		ProcessPtr writer(getProcessPtr(_writer));
               
            ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel shared mutex claimed",__LINE__);
            ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex claimed",__LINE__);
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
			ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
			ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
			
			
			{
				{
					int n;
					ScopedExtInput<int> extInput(c.reader(),&n);
					
					ASSERTEQ(writer,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
					ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
					ASSERTEQ(true,c.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
					ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,c.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
								
			}
			
			//Let the writer finish:
			CPPCSP_Yield();
			
			//now let the reader arrive first:
			
			_writer = new WriterProcess<int>(d.writer(),0,1);
    		writer = getProcessPtr(_writer);
    		
    		EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We fork them
				(us,NullProcessPtr,NullProcessPtr) //We block on the channel
				(writer,us,us) //They arrive and free us
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
				
				(us,writer,writer) //We end the extended input and free them				
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
    			forking.forkInThisThread(_writer);
    			
    			ASSERTEQ(false,d.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
    			{
    				int n;
    				ScopedExtInput<int> extInput(d.reader(),&n);
    				//we will block, but then the writer will run, block, and free us
    				
    				ASSERTEQ(writer,d.waiting,"Channel data (waiting) not as expected",__LINE__);			
					ASSERTEQ(&(_writer->t),ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
					ASSERTEQ(true,d.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
					ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected",__LINE__);			
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(false,d.readerMutex.isClaimed(),"Channel shared mutex not claimed",__LINE__);
				ASSERTEQ(false,d.mutex.isClaimed(),"Channel mutex not claimed",__LINE__);				
			}						
		
			ASSERTEQ(expB,actB,"Events not as expected",__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Any2* Unbuffered Extended Test 0");
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////
	

	//TODO later test multi-threading	?				
	
	/**
	*	Tests poison on the any2* channels.  It particularly checks that the shared-end mutex is released properly
	*	when poison is encountered
	*/
	template <typename CHANNEL>
	static TestResult testPoisonAny2()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c,d,e;
		ScopedForking forking;
		
		ASSERTEQ(false,c.isPoisoned/*ACCESS(_One2OneChannel<int>,c,isPoisoned)*/,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		c.writer().poison();
		
		ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		//test release of mutex when poison is noticed (either fresh or part-way through):
		
		//fresh, by writer:
		try
		{			
			c.writer() << 6;
			
			ASSERTL(false,"Poison was not noticed!",__LINE__); 
		}
		catch (PoisonException)
		{
		}
		
		ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		//test poison part-way through a write (but make sure we don't then make a poison call that could alter things):
		
		EventList expA;
			
		CSProcessPtr _poisoner = new ChannelPoisoner<Chanin<int> >(d.reader());
		ProcessPtr poisoner = getProcessPtr(_poisoner);
		CSProcessPtr _writer = new WriterProcess<int>(d.writer(),5);
		ProcessPtr writer = getProcessPtr(_writer);
			
		expA = tuple_list_of
			(us,poisoner,poisoner) (us,writer,writer) //We fork off the processes
			(us,NullProcessPtr,NullProcessPtr) //we block trying to write from the channel
			(poisoner,us,us) //We are freed as part of the poison
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The poisoner finishes
			(writer,NullProcessPtr,NullProcessPtr) //The writer starts, and blocks waiting for the shared-end mutex
			(us,writer,writer) //We wake up and pass on the mutex to the writer
		;			
		
		int n = 7;
		EventList actA;		
		{
			
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_poisoner);
			
			//This way, we will be poisoned after we have blocked:
			//And there will be also be another process queued up after us:
			
			forking.forkInThisThread(_writer);
			
			try
			{				
				d.writer() << n;
				
				ASSERTL(false,"Poison was not noticed!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		ASSERTEQ(expA,actA,"Events were not as expected",__LINE__);
		
		ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(&n,ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(true,d.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		EventList expB;					
			
		expB = tuple_list_of
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer sees the poison, and finishes			
		;			
			
		EventList actB;
		{			
			RecordEvents _(&actB);
		
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Events were not as expected",__LINE__);
		
		ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(&n,ACCESS(_One2OneChannel<int>,d,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,d.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
						
		//farm a writer then a poisoner, then read successfully:
		
		_poisoner = new ChannelPoisoner<Chanout<int> >(e.writer());
		poisoner = getProcessPtr(_poisoner);
		_writer = new WriterProcess<int>(e.writer(),5);
		writer = getProcessPtr(_writer);
		
		EventList expC;
		expC = tuple_list_of
			(us,writer,writer) (us,poisoner,poisoner) //We fork off the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			
			(writer,NullProcessPtr,NullProcessPtr) //The writer blocks on the channel
			(poisoner,NullProcessPtr,NullProcessPtr) //The poisoner blocks on the mutex
			
			(us,writer,writer) //We read from the channel, freeing the writer			
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(writer,poisoner,poisoner) //The writer grants the poisoner the mutex
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
		;			
		
		EventList actC;
		{			
			RecordEvents _(&actC);
			
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_poisoner);
			
			CPPCSP_Yield();
			
			try
			{
				e.reader() >> n;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison unexpectedly found!",__LINE__);
			}
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expC,actC,"Events not as expected",__LINE__);
		
		ASSERTEQ(false,e.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,e.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(true,e.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		CPPCSP_Yield(); //gives the poisoner a chance to poison
		
		ASSERTEQ(true,e.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,e.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(false,e.writerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		END_TEST(ChannelName<CHANNEL>::Name() +  " Unbuffered Any2* Poison Test");
	}
	
	/////////////////////////////////////////////////////////////////////////////////////
	
	
	/**
	*	Tests poison on the *2any channels.  It particularly checks that the shared-end mutex is released properly
	*	when poison is encountered
	*/
	template <typename CHANNEL>
	static TestResult testPoison2Any()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c,d,e;
		ScopedForking forking;
		
		ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		c.reader().poison();
		
		ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		//test release of mutex when poison is noticed (either fresh or part-way through):
		
		int n;
		
		//fresh, by reader:
		try
		{			
			c.reader() >> n;
			
			ASSERTL(false,"Poison was not noticed!",__LINE__); 
		}
		catch (PoisonException)
		{
		}
		
		ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,c.waiting,"Channel data (waiting) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,c,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(false,c.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		//test poison part-way through a write (but make sure we don't then make a poison call that could alter things):
		
		EventList expA;
			
		CSProcessPtr _poisoner = new ChannelPoisoner<Chanout<int> >(d.writer());
		ProcessPtr poisoner = getProcessPtr(_poisoner);
		CSProcessPtr _reader = new ReaderProcess<int>(d.reader(),5);
		ProcessPtr reader = getProcessPtr(_reader);
			
		expA = tuple_list_of
			(us,poisoner,poisoner) (us,reader,reader) //We fork off the processes
			(us,NullProcessPtr,NullProcessPtr) //we block trying to read from the channel
			(poisoner,us,us) //We are freed as part of the poison
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The poisoner finishes
			(reader,NullProcessPtr,NullProcessPtr) //The reader starts, and blocks waiting for the shared-end mutex
			(us,reader,reader) //We wake up and pass on the mutex to the reader
		;			
				
		EventList actA;		
		{
			
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_poisoner);
			
			//This way, we will be poisoned after we have blocked:
			//And there will be also be another process queued up after us:
			
			forking.forkInThisThread(_reader);
			
			try
			{				
				d.reader() >> n;
				
				ASSERTL(false,"Poison was not noticed!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		ASSERTEQ(expA,actA,"Events were not as expected",__LINE__);
		
		ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(&n,ACCESS(_One2OneChannel<int>,d,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(true,d.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		EventList expB;					
			
		expB = tuple_list_of
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The reader sees the poison, and finishes			
		;			
			
		EventList actB;
		{			
			RecordEvents _(&actB);
		
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Events were not as expected",__LINE__);
		
		ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,d.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(&n,ACCESS(_One2OneChannel<int>,d,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(false,d.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
						
		//farm a reader then a poisoner, then write successfully:
		
		_poisoner = new ChannelPoisoner<Chanin<int> >(e.reader());
		poisoner = getProcessPtr(_poisoner);
		_reader = new ReaderProcess<int>(e.reader());
		reader = getProcessPtr(_reader);
		
		EventList expC;
		expC = tuple_list_of
			(us,reader,reader) (us,poisoner,poisoner) //We fork off the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			
			(reader,NullProcessPtr,NullProcessPtr) //The reader blocks on the channel
			(poisoner,NullProcessPtr,NullProcessPtr) //The poisoner blocks on the mutex
			
			(us,reader,reader) //We read from the channel, freeing the reader
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(reader,poisoner,poisoner) //The reader grants the poisoner the mutex
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The reader finishes
		;			
		
		EventList actC;
		{			
			RecordEvents _(&actC);
			
			forking.forkInThisThread(_reader);
			forking.forkInThisThread(_poisoner);
			
			CPPCSP_Yield();
			
			try
			{
				e.writer() << n;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison unexpectedly found!",__LINE__);
			}
			
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expC,actC,"Events not as expected",__LINE__);
		
		ASSERTEQ(false,e.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,e.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(true,e.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		CPPCSP_Yield(); //gives the poisoner a chance to poison
		
		ASSERTEQ(true,e.isPoisoned,"Channel data (isPoisoned) not as expected)",__LINE__);
		ASSERTEQ(NullProcessPtr,e.waiting,"Channel data (waiting) not as expected)",__LINE__);		
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,src),"Channel data (src) not as expected)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(_One2OneChannel<int>,e,dest),"Channel data (dest) not as expected)",__LINE__);
		ASSERTEQ(false,e.readerMutex.isClaimed(),"Channel data (mutex-claimed) not as expected)",__LINE__);
		
		END_TEST(ChannelName<CHANNEL>::Name() +  " Any2* Unbuffered Poison Test");
	}
	
	
	/////////////////////////////////////////////////////////////////////////////////////
	
	/**	
	*	Tests for Sputh's poison oversight, where a communication completes successfully
	*	but before one party has woken up, the channel is poisoned.  If the implementation is done
	*	badly, the party will see this poison as being part of the successful communication, and will
	*	throw a poison exception that it really should not throw
	*/
	template <typename CHANNEL, bool BUFFERED>
	static TestResult testLatePoison()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		CHANNEL c,d,e;
		BlackHoleChannel<int> hole;
		ScopedForking forking;
		
		
		CSProcessPtr _poisoner = new ChannelPoisoner< Chanout<int> >(c.writer());
		ProcessPtr poisoner = getProcessPtr(_poisoner);
		CSProcessPtr _writer = new WriterProcess<int>(c.writer(),5);
		ProcessPtr writer = getProcessPtr(_writer);
		
		EventList expA;
		expA = tuple_list_of
			(us,writer,writer) (us,poisoner,poisoner) //We fork off the processes
			(us,NullProcessPtr,NullProcessPtr) //We read from the channel
			
			(writer,us,us) //The writer writes to the channel, freeing us
			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The poisoner finishes
		;			
		
		EventList actA;
		{			
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_poisoner);
			
			try
			{
				int n;
				c.reader() >> n;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison detected when it should not have been!",__LINE__);
			}		
			
			try
			{
				c.reader().checkPoison();
				
				ASSERTL(false,"Poison NOT detected when it should have been!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		ASSERTEQ(expA,actA,"Events not as expected",__LINE__);
		ASSERTEQ(true,c.isPoisoned,"Channel not poisoned",__LINE__);
		
		_poisoner = new ChannelPoisoner< Chanin<int> >(d.reader());
		poisoner = getProcessPtr(_poisoner);
		CSProcessPtr _reader = new ReaderProcess<int>(d.reader());
		ProcessPtr reader = getProcessPtr(_reader);
		
		EventList expB;
		expB = tuple_list_of
			(us,reader,reader) (us,poisoner,poisoner) //We fork off the processes
			(us,NullProcessPtr,NullProcessPtr) //We write to the channel
			
			(reader,us,us) //The reader reads from the channel, freeing us
			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The reader finishes
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The poisoner finishes
		;			
		
		EventList actB;
		{			
			RecordEvents _(&actB);
			
			forking.forkInThisThread(_reader);
			forking.forkInThisThread(_poisoner);
			
			try
			{				
				d.writer() << 8;
				if (BUFFERED)
					d.writer() << 8;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison detected when it should not have been!",__LINE__);
			}								
				
			
			try
			{
				d.writer().checkPoison();
				
				ASSERTL(false,"Poison NOT detected when it should have been!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		ASSERTEQ(expB,actB,"Events not as expected",__LINE__);
		ASSERTEQ(true,d.isPoisoned,"Channel not poisoned",__LINE__);
		
		_poisoner = new ChannelPoisoner< Chanin<int> >(e.reader());
		poisoner = getProcessPtr(_poisoner);
		_reader = new ExtReader<int>(e.reader()); //an empty extended read
		reader = getProcessPtr(_reader);
		
		EventList expC;
		expC = tuple_list_of
			(us,reader,reader) (us,poisoner,poisoner) //We fork off the processes
			(us,NullProcessPtr,NullProcessPtr) //We write to the channel
			
			(reader,us,us) //The reader reads from the channel, freeing us
			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The reader finishes
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The poisoner finishes
		;			
		
		EventList actC;
		{			
			RecordEvents _(&actC);
			
			forking.forkInThisThread(_reader);
			forking.forkInThisThread(_poisoner);
			
			try
			{				
				e.writer() << 8;
				if (BUFFERED)
					e.writer() << 8;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison detected when it should not have been!",__LINE__);
			}		
			
			try
			{
				e.writer().checkPoison();
				
				ASSERTL(false,"Poison NOT detected when it should have been!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		ASSERTEQ(expC,actC,"Events not as expected",__LINE__);
		ASSERTEQ(true,e.isPoisoned,"Channel not poisoned",__LINE__);
		
		END_TEST(ChannelName<CHANNEL>::Name() + " late poison test");
	}		

	std::list<TestResult (*)()> tests()
	{
		#define CHANNELS (One2OneChannel<int>)(One2AnyChannel<int>)(Any2OneChannel<int>)(Any2AnyChannel<int>)
						
		us = currentProcess();
		return list_of<TestResult (*) ()>
			TEST_FOR_EACH(test0, CHANNELS )
			TEST_FOR_EACH(test1, CHANNELS )
			TEST_FOR_EACH(testPoison0, CHANNELS )			
			
			TEST_FOR_EACH(testExtended0, CHANNELS )
			TEST_FOR_EACH(testExtended1, CHANNELS )
			TEST_FOR_EACH(testExtendedPoison, CHANNELS )
			TEST_FOR_EACH(testExtException, CHANNELS )						
			
		#ifdef CPPCSP_PARCOMM
			(testParcomm)
		#endif
			(testAny20<Any2OneChannel<int> >) (testAny20<Any2AnyChannel<int> >)
			(test2Any0<One2AnyChannel<int> >) (test2Any0<Any2AnyChannel<int> >)
			
			(testExtAny20<Any2OneChannel<int> >) (testExtAny20<Any2AnyChannel<int> >)
			(testExt2Any0<One2AnyChannel<int> >) (testExt2Any0<Any2AnyChannel<int> >)
			
			(testPoisonAny2<Any2OneChannel<int> >) (testPoisonAny2<Any2AnyChannel<int> >)
			
			(testPoison2Any<One2AnyChannel<int> >) (testPoison2Any<Any2AnyChannel<int> >)
			
			(testLatePoison< One2OneChannel<int> , false >) 
			(testLatePoison< Any2AnyChannel<int> , false >)
			(testLatePoison< Any2OneChannel<int> , false >)
			(testLatePoison< One2AnyChannel<int> , false >)
			
			(testLatePoison< FIFO1<BufferedOne2OneChannel<int> > , true >) 
			(testLatePoison< FIFO1<BufferedAny2AnyChannel<int> > , true >)
			(testLatePoison< FIFO1<BufferedAny2OneChannel<int> > , true >)
			(testLatePoison< FIFO1<BufferedOne2AnyChannel<int> > , true >)
		;
	}
	
	std::list<TestResult (*)()> perfTests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>(commsTimeTest0) (commsTimeTest1) (commsTimeTest2) (commsTimeTest3) 
			(commsTimeTest4) (commsTimeTest5) (commsTimeTest6) /*(commsTimeTest7) (commsTimeTest8) (commsTimeTest8B)*/
			(commsTimeTest9) (commsTimeTest10)
			(commsTimeTest0Double) (commsTimeTest1Double)
			(commsTimeTest0Triangle) (commsTimeTest1Triangle) (commsTimeTest2Triangle)
			(pingPongTest0) (pingPongTest1) (pingPongTest2) //(pingPongTest3)
		;
	}	
	
	inline virtual ~ChannelTest()
	{
	}
};

ProcessPtr ChannelTest::us;

//} //namespace 

Test* GetChannelTest()
{
	return new ChannelTest;
}
