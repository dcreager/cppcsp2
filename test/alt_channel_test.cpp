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


class StressedAlter : public CSProcess
{
private:
	vector< AltChanin<int> > ins;
	Chanout<Time> out;
	const unsigned reads;
protected:
	void run()
	{
		vector< Guard* > guards(ins.size());
		for (unsigned i = 0;i < ins.size();i++)
		{
			guards[i] = ins[i].inputGuard();
		}
		Alternative alt(guards);
		
		for (unsigned i = 0;i < reads / 100;i++)
		{
			int n;
			ins[alt.fairSelect()] >> n;			
		}
		
		csp::Time t0,t1;
		CurrentTime(&t0);
		for (unsigned i = 0;i < reads;i++)
		{
			int n;
			ins[alt.fairSelect()] >> n;			
		}
		CurrentTime(&t1);
		t1 -= t0;
		
		for (unsigned i = 0;i < ins.size();i++)
		{
			ins[i].poison();
		}
		
		out << t1;
	}
public:
	StressedAlter(const vector< AltChanin<int> >& _ins,const Chanout<Time>& _out,const unsigned _reads)
		:	ins(_ins),out(_out),reads(_reads)
	{
	}
};

class TripleAlter : public CSProcess
{
private:
	AltChanin<int> in;
	Chanout<int> out;
protected:
	void run()
	{
		list<Guard*> guards;
		guards.push_back(in.inputGuard());
		guards.push_back(in.inputGuard());
		guards.push_back(in.inputGuard());
		
		Alternative alt(guards);
		
		int n = alt.priSelect();
		int dontcare;
		in >> dontcare;
		out << n;
	}
public:
	TripleAlter(const AltChanin<int>& _in,const Chanout<int>& _out)
		:	in(_in),out(_out)
	{
	}
};

class AltChannelTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;
	
	static TestResult testNormal0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		One2OneChannel<int> c0,c1;
		One2OneChannel<int> d;
		
		ScopedForking forking;
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) //We fork them
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
				
			forking.forkInThisThread(_alter);
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(alter,c0.waiting,"Channel data (waiting) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waiting,"Channel data (waiting) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));
    	
    	EventList expB;
			
		expB = tuple_list_of
			(us,writer,writer) //We fork them
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(writer,alter,alter) //They free the alter
			(writer,NullProcessPtr,NullProcessPtr) //They must wait on the channel
		;			
			
		EventList actB;
		{
			RecordEvents _(&actB);
				
			forking.forkInThisThread(_writer);
				
			CPPCSP_Yield();
		}
		
		//We will yield, and add ourselves to the queue.  Writer will run and add the alter to the run queue
		//So when the writer then finishes, it will be us that gets scheduled again
		
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(writer,c0.waiting,"Channel data (waiting) not as expected (A,0)",__LINE__);
		ASSERTEQ(&(_writer->t),c0.src,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waiting,"Channel data (waiting) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);						
		
		EventList expC;
			
		expC = tuple_list_of			
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,writer,writer) //They complete the communication and free the writer
			(alter,NullProcessPtr,NullProcessPtr) //They block writing to their output channel
		;			
			
		EventList actC;
		{
			RecordEvents _(&actC);			
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected (C,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.src,"Channel data (dest) not as expected (C,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (C,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (C,1)",__LINE__);
		
		
		_writer = new WriterProcess<int>(c0.writer(),40,1);
    	writer = getProcessPtr(_writer);
    	WriterProcess<int>* _writer1 = new WriterProcess<int>(c1.writer(),41,1);
    	ProcessPtr writer1(getProcessPtr(_writer1));
		
		EventList expD;			
			
		expD = tuple_list_of
			(us,writer,writer) (us,writer1,writer1) //We spawn two new writers
			(us,alter,alter) //We free the alter
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //In the midst of this, the previous writer finishes!
			(writer,NullProcessPtr,NullProcessPtr) (writer1,NullProcessPtr,NullProcessPtr) //The writers block on the channels
			(alter,writer1,writer1) //The alter selects writer1 (fair select, last time it chose 0)
			(alter,NullProcessPtr,NullProcessPtr) //The alter blocks on its output channel
		;			
			
		int n;
			
		EventList actD;
		{
			RecordEvents _(&actD);			
				
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_writer1);
				
			d.reader() >> n;
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expD,actD,"Channel events not as expected in part D",__LINE__);
		ASSERTEQ(writer,c0.waiting,"Channel data (waiting) not as expected (D,0)",__LINE__);
		ASSERTEQ(&(_writer->t),c0.src,"Channel data (dest) not as expected (D,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (D,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (D,1)",__LINE__);
		ASSERTEQ(alter,d.waiting,"Channel data (waiting) not as expected (D,2)",__LINE__);
		ASSERTEQ(42,n,"Read value was not as expected (D)",__LINE__);
				
		//This will shut everyone down:
		d.reader().poison();
		
		CPPCSP_Yield();
		CPPCSP_Yield();
		
		END_TEST("Test of Alting over Unbuffered Channels, General");
	}
	
	//Tests poison being there before the alter arrives
	
	static TestResult testNormal1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		One2OneChannel<int> c0,c1;
		One2OneChannel<int> d;
		
		ScopedForking forking;
		
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));
    	
    	c1.writer().poison();
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,writer,writer) (us,alter,alter) //We fork the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			(alter,writer,writer) //They free the writer by selecting its guard
			(alter,NullProcessPtr,NullProcessPtr) //They block on their output channel
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
				
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_alter);
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
		ASSERTEQ(alter,d.waiting,"Channel data(waiting) not as expected (A,2)",__LINE__);
    	
    	EventList expB;
			
		expB = tuple_list_of
			(us,alter,alter) //We free the alter
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The alter selects the poisoned guard, poisons everything and finishes
		;			
					
		EventList actB;
		{
			RecordEvents _(&actB);
				
			int n;
			d.reader() >> n;
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected (B,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (B,0)",__LINE__);
		ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected (B,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (B,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (B,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (B,1)",__LINE__);
	
		CPPCSP_Yield();
		CPPCSP_Yield();
		
		END_TEST("Test of Alting over Unbuffered Channels, poisoning before the Alt");
	}
	
	//Tests poison arriving at the same time as something else, and making sure the correct one is chosen:
	
	static TestResult testNormal2()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		One2OneChannel<int> c0,c1;
		One2OneChannel<int> d;
		
		ScopedForking forking;
		
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));    	
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) (us,writer,writer) //We fork the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
			(writer,alter,alter) //They write to the channel and wake the alter
			(writer,NullProcessPtr,NullProcessPtr) //They have to wait for the alter			
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);	
			forking.forkInThisThread(_writer);
							
			CPPCSP_Yield();			
		}
				
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(writer,c0.waiting,"Channel data (waiting) not as expected (A,0)",__LINE__);
		ASSERTEQ(&(_writer->t),c0.src,"Channel data (src) not as expected (A,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waiting,"Channel data (waiting) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(false,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
	
		EventList expB;
			//We should not affect the run queue when we poison, because the alter has already awoken
		
		EventList actB;
		{
			RecordEvents _(&actB);
			
			c1.writer().poison();
		}
				
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(writer,c0.waiting,"Channel data (waiting) not as expected (B,0)",__LINE__);
		ASSERTEQ(&(_writer->t),c0.src,"Channel data (src) not as expected (B,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (B,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (B,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (B,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (B,1)",__LINE__);
    	
    	EventList expC;
    	expC = tuple_list_of			
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,writer,writer) //They free the writer by using that guard
			(alter,NullProcessPtr,NullProcessPtr) //They block on their output channel			
		;			
			
		
		EventList actC;
		{
			RecordEvents _(&actC);
			
			CPPCSP_Yield();
		}
				
		ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected (C,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.src,"Channel data (src) not as expected (C,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (C,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (C,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (C,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (C,1)",__LINE__);
    	ASSERTEQ(alter,d.waiting,"Channel data (waiting) not as expected (C,2)",__LINE__);
    	
    	//This will get everything shut down:
    	d.reader().poison();
    	
    	CPPCSP_Yield();
		CPPCSP_Yield();
    	
    	END_TEST("Test of Alting over Unbuffered Channels, poisoning before the Alt after a writer has already arrived");
    }
	
	//test poison only, while alter is already waiting:
	
	static TestResult testNormal3()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		One2OneChannel<int> c0,c1;
		One2OneChannel<int> d;
		
		ScopedForking forking;		
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) //We fork the process
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
			(us,alter,alter) //We poison both channels, but they only wake up once
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);				
							
			CPPCSP_Yield();
			
			c1.writer().poison();
			c0.writer().poison();
		}
				
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waiting,"Channel data (waiting) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waiting,"Channel data (waiting) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
	
		CPPCSP_Yield();
		CPPCSP_Yield();
	
		END_TEST("Test of Alting over Unbuffered Channels, poisoning after the Alt has started");
	}
	
	//test pending methods while poisoned and not poisoned (on ordinary channels):
	
	static TestResult testNormal4()
	{
		One2OneChannel<int> c;
		AltChanin<int> cIn(c.reader());
	
		{
			ScopedForking forking;
			BEGIN_TEST()
		
			SetUp setup;			
		
			ASSERTEQ(false,cIn.pending(),"Empty channel should not return true from pending()",__LINE__);
		
			forking.forkInThisThread(new WriterProcess<int>(c.writer(),7));
		
			CPPCSP_Yield();
		
			ASSERTEQ(true,cIn.pending(),"Channel with writer waiting should return true from pending()",__LINE__);
		
			int n;
			cIn >> n;
		
			ASSERTEQ(false,cIn.pending(),"Empty channel should not return true from pending()",__LINE__);
		
			cIn.poison();
		
			ASSERTEQ(true,cIn.pending(),"Poisoned channel should return true from pending()",__LINE__);
				
			END_TEST_C("Test of pending() on unbuffered channels",cIn.poison());
		}
	}	

	//test alting over buffered channels (and pending on buffered channels):
	//First, it tests that an alter will block on a channel with an empty buffer
	//Second it tests that writing to this channel will wake the alter
	//Then it tests fair selection of writers on buffered channels

	static TestResult testBuf0()
	{
		FIFOBuffer<int>::Factory factory(2);		
		BufferedOne2OneChannel<int> c0(factory),c1(factory);
		One2OneChannel<int> d;
		ScopedForking forking;
	
		BEGIN_TEST()
		
		SetUp setup;
		
		
				
		
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) //We fork them
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
				
			forking.forkInThisThread(_alter);
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(alter,c0.waitingProcess,"Channel data (waitingProcess) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waitingProcess,"Channel data (waitingProcess) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));
    	
    	EventList expB;
			
		expB = tuple_list_of
			(us,writer,writer) //We fork them
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(writer,alter,alter) //They free the alter
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
		;			
			
		EventList actB;
		{
			RecordEvents _(&actB);
				
			forking.forkInThisThread(_writer);
				
			CPPCSP_Yield();
		}				
		
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.src,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waitingProcess,"Channel data (waitingProcess) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);						
		//ASSERTEQ((FIFOBuffer<int>::LIST_DATA_TYPE)boost::assign::list_of<int>(42),((FIFOBuffer<int>*)c0.buffer)->buffer,"Channel buffer not as expected",__LINE__);
		ASSERTEQ(false,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
		
		EventList expC;
			
		expC = tuple_list_of			
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block writing to their output channel
		;			
			
		EventList actC;
		{
			RecordEvents _(&actC);			
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (C,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.src,"Channel data (dest) not as expected (C,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (C,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (C,1)",__LINE__);
		ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
		
		
		_writer = new WriterProcess<int>(c0.writer(),40,1);
    	writer = getProcessPtr(_writer);
    	WriterProcess<int>* _writer1 = new WriterProcess<int>(c1.writer(),41,1);
    	ProcessPtr writer1(getProcessPtr(_writer1));
		
		EventList expD;			
			
		expD = tuple_list_of
			(us,writer,writer) (us,writer1,writer1) //We spawn two new writers
			(us,alter,alter) //We free the alter
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) (NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writers finish
			//The alter selects writer1 (fair select, last time it chose 0)
			(alter,NullProcessPtr,NullProcessPtr) //The alter blocks on its output channel
		;			
			
		int n;
			
		EventList actD;
		{
			RecordEvents _(&actD);			
				
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_writer1);
				
			d.reader() >> n;
				
			CPPCSP_Yield();
		}		
		
		ASSERTEQ(expD,actD,"Channel events not as expected in part D",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (D,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.src,"Channel data (dest) not as expected (D,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (D,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (D,1)",__LINE__);
		ASSERTEQ(false,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
		ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
		//ASSERTEQ((FIFOBuffer<int>::LIST_DATA_TYPE)boost::assign::list_of<int>(40),((FIFOBuffer<int>*)c1.buffer)->buffer,"Channel buffer not as expected",__LINE__);		
		ASSERTEQ(alter,d.waiting,"Channel data (waiting) not as expected (D,2)",__LINE__);
		ASSERTEQ(42,n,"Read value was not as expected (D)",__LINE__);
				
		//This will shut everyone down:
		d.reader().poison();
		
		CPPCSP_Yield();
		CPPCSP_Yield();
		
		END_TEST_C("Test of Alting over Buffered Channels, General", d.reader().poison();c0.writer().poison();c1.writer().poison() );
	}
	
	//Tests poison being there before the alter arrives
	
	static TestResult testBuf1()
	{		
		FIFOBuffer<int>::Factory factory(2);
		
		BufferedOne2OneChannel<int> c0(factory),c1(factory);
		One2OneChannel<int> d;
	
		ScopedForking forking;
		
		BEGIN_TEST()
		
		SetUp setup;
		
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));
    	
    	c1.writer().poison();
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,writer,writer) (us,alter,alter) //We fork the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer finishes			
			(alter,NullProcessPtr,NullProcessPtr) //They block on their output channel
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
				
			forking.forkInThisThread(_writer);
			forking.forkInThisThread(_alter);
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
		ASSERTEQ(alter,d.waiting,"Channel data(waiting) not as expected (A,2)",__LINE__);
    	
    	EventList expB;
			
		expB = tuple_list_of
			(us,alter,alter) //We free the alter
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield			
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The alter selects the poisoned guard, poisons everything and finishes
		;			
					
		EventList actB;
		{
			RecordEvents _(&actB);
				
			int n;
			d.reader() >> n;
				
			CPPCSP_Yield();
		}
		
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (B,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (B,0)",__LINE__);
		ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected (B,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (B,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (B,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (B,1)",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c0.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
    	ASSERTEQ(true,(static_cast<FIFOBuffer<int>*>(c1.buffer))->buffer.empty(),"Channel buffer not as expected",__LINE__);
	
		CPPCSP_Yield();
		CPPCSP_Yield();
		
		END_TEST_C("Test of Alting over Buffered Channels, poisoning before the Alt", d.reader().poison();c0.writer().poison();c1.writer().poison() );
	}
	
	//Tests poison arriving at the same time as something else, and making sure the correct one is chosen:
	
	static TestResult testBuf2()
	{
		FIFOBuffer<int>::Factory factory(2);
		
		BufferedOne2OneChannel<int> c0(factory),c1(factory);
		One2OneChannel<int> d;
	
		ScopedForking forking;
	
		BEGIN_TEST()
		
		SetUp setup;		
					
		WriterProcess<int>* _writer = new WriterProcess<int>(c0.writer(),42,1);
    	ProcessPtr writer(getProcessPtr(_writer));
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) (us,writer,writer) //We fork the processes
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
			(writer,alter,alter) //They write to the channel and wake the alter
			(NullProcessPtr,NullProcessPtr,NullProcessPtr) //The writer can finish
		;			
			
		EventList actA;
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);	
			forking.forkInThisThread(_writer);
							
			CPPCSP_Yield();			
		}
				
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(alter,c1.waitingProcess,"Channel data (waitingProcess) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(false,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
	
		EventList expB;
			//We should not affect the run queue when we poison, because the alter has already awoken
		
		EventList actB;
		{
			RecordEvents _(&actB);
			
			c1.writer().poison();
		}
				
		ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (B,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (B,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (B,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (B,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (B,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (B,1)",__LINE__);
    	
    	EventList expC;
    	expC = tuple_list_of			
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield			
			(alter,NullProcessPtr,NullProcessPtr) //They block on their output channel			
		;			
			
		
		EventList actC;
		{
			RecordEvents _(&actC);
			
			CPPCSP_Yield();
		}
				
		ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (C,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (C,0)",__LINE__);
		ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected (C,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (C,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (C,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (C,1)",__LINE__);
    	ASSERTEQ(alter,d.waiting,"Channel data (waitingProcess) not as expected (C,2)",__LINE__);
    	
    	//This will get everything shut down:
    	d.reader().poison();
    	
    	CPPCSP_Yield();
		CPPCSP_Yield();
    	
    	END_TEST_C("Test of Alting over Buffered Channels, poisoning before the Alt after a writer has already arrived",c1.writer().poison();d.reader().poison(););
    }
	
	//test poison only, while alter is already waiting:
	
	static TestResult testBuf3()
	{
		BEGIN_TEST()
		
		SetUp setup;

		FIFOBuffer<int>::Factory factory(2);
		
		BufferedOne2OneChannel<int> c0(factory),c1(factory);
		BufferedOne2OneChannel<int> d(factory);

		ScopedForking forking;		
			
	    Merger<int>* _alter = new Merger<int>(c0.reader(),c1.reader(),d.writer());
    	ProcessPtr alter(getProcessPtr(_alter));
    	
    	EventList expA;
			
		expA = tuple_list_of
			(us,alter,alter) //We fork the process
			(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
			(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
			(us,alter,alter) //We poison both channels, but they only wake up once
		;
			
		EventList actA;
		{
			RecordEvents _(&actA);
			
			forking.forkInThisThread(_alter);				
							
			CPPCSP_Yield();
			
			c1.writer().poison();
			c0.writer().poison();
		}
				
		ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
		ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waitingProcess) not as expected (A,0)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c0.dest,"Channel data (dest) not as expected (A,0)",__LINE__);
		ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected (A,0)",__LINE__);
		ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waitingProcess) not as expected (A,1)",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),c1.dest,"Channel data (dest) not as expected (A,1)",__LINE__);
    	ASSERTEQ(true,c1.isPoisoned,"Channel data (isPoisoned) not as expected (A,1)",__LINE__);
	
		CPPCSP_Yield();
		CPPCSP_Yield();
	
		END_TEST("Test of Alting over Buffered Channels, poisoning after the Alt has started");
	}
	
	//test pending methods while poisoned and not poisoned (on buffered channels):
	
	static TestResult testBuf4()
	{
		FIFOBuffer<int>::Factory factory(2);
		
		BufferedOne2OneChannel<int> c(factory);

		AltChanin<int> cIn(c.reader());
	
		{
			ScopedForking forking;
			BEGIN_TEST()
		
			SetUp setup;			
		
			ASSERTEQ(false,cIn.pending(),"Empty channel should not return true from pending()",__LINE__);
		
			forking.forkInThisThread(new WriterProcess<int>(c.writer(),7));
		
			CPPCSP_Yield();
		
			ASSERTEQ(true,cIn.pending(),"Channel with writer waitingProcess should return true from pending()",__LINE__);
		
			int n;
			cIn >> n;
		
			ASSERTEQ(false,cIn.pending(),"Empty channel should not return true from pending()",__LINE__);
		
			cIn.poison();
		
			ASSERTEQ(true,cIn.pending(),"Poisoned channel should return true from pending()",__LINE__);
				
			END_TEST_C("Test of pending() on Buffered channels",cIn.poison());
		}
	}		
	
	static TestResult testRepGuard0()
	{
		One2OneChannel<int> c;
		
		FIFOBuffer<int>::Factory factory(1);
		BufferedOne2OneChannel<int> d(factory);		

		{
			ScopedForking forking;
			BEGIN_TEST()
			
			SetUp setup;
			
			CSProcess* _alter = new TripleAlter(c.reader(),d.writer());
			ProcessPtr alter(getProcessPtr(_alter));
			
	    	EventList expA;
			
			expA = tuple_list_of
				(us,alter,alter) //We fork the process
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(alter,NullProcessPtr,NullProcessPtr) //They block on the channels
			;			
						
			ASSERTEQ(NullProcessPtr,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),c.dest,"dest not as expected",__LINE__);						
			
			EventList actA;
			{
				RecordEvents _(&actA);
			
				forking.forkInThisThread(_alter);				
							
				CPPCSP_Yield();
			}
			
			//Now enabled:
			ASSERTEQ(expA,actA,"Events not as expected in part A",__LINE__);
			ASSERTEQ(alter,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),c.dest,"dest not as expected",__LINE__);

			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),42,1);
    		ProcessPtr writer(getProcessPtr(_writer));
			
			forking.forkInThisThread(_writer);
			
			CPPCSP_Yield();
			
			//Now the writer has been:
			ASSERTEQ(writer,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),c.src,"dest not as expected",__LINE__);			
			
			CPPCSP_Yield();
			
			//Now disabled:
			ASSERTEQ(NullProcessPtr,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),c.dest,"dest not as expected",__LINE__);
			
			int n;
			d.reader() >> n;
			ASSERTEQ(0,n,"First guard was not selected in pri alt",__LINE__);
		
			END_TEST_C("Test repeated channel guards 0 (unbuffered)",c.reader().poison());
		}
	}
	
	static TestResult testRepGuard1()
	{
		One2OneChannel<int> c;

		FIFOBuffer<int>::Factory factory(1);
		BufferedOne2OneChannel<int> d(factory);		


		{
			ScopedForking forking;
			BEGIN_TEST()
			
			SetUp setup;

			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),42,1);
    		ProcessPtr writer(getProcessPtr(_writer));
			
			forking.forkInThisThread(_writer);
			
			CPPCSP_Yield();
			
			//The writer is waiting:
			ASSERTEQ(writer,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),c.src,"dest not as expected",__LINE__);			


			
			CSProcess* _alter = new TripleAlter(c.reader(),d.writer());
			ProcessPtr alter(getProcessPtr(_alter));
			
	    	EventList expA;
			
			expA = tuple_list_of
				(us,alter,alter) //We fork the process
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(alter,writer,writer) //They free the writer
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish
			;			
						
			EventList actA;
			{
				RecordEvents _(&actA);
			
				forking.forkInThisThread(_alter);				
							
				CPPCSP_Yield();
			}
			
			//Now finished:
			ASSERTEQ(expA,actA,"Events not as expected in part A",__LINE__);
			ASSERTEQ(NullProcessPtr,c.waiting,"waiting not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),c.dest,"dest not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),c.src,"src not as expected",__LINE__);

			int n;
			d.reader() >> n;
			ASSERTEQ(0,n,"First guard was not selected in pri alt",__LINE__);		
		
			END_TEST_C("Test repeated channel guards 1 (unbuffered)",c.reader().poison());
		}
	}	
	
	template <unsigned CHANNELS,unsigned WRITERS_PER_CHANNEL, unsigned COMMS_EACH, bool KERNEL_THREADS, typename CHANNEL_FACTORY>
	static TestResult testStressedAlt()
	{
		CHANNEL_FACTORY factory;
		vector< AltChanin<int> > ins(CHANNELS);
		One2OneChannel<Time> result;
		
		list< CSProcessPtr > processes;				
		
		for (unsigned i = 0;i < CHANNELS;i++)
		{
			pair< AltChanin<int> , Chanout<int> > pr = factory.any2OnePair();
			ins[i] = pr.first;
			
			for (unsigned j = 0;j < WRITERS_PER_CHANNEL;j++)
			{
				//They will be poisoned anyway, so just tell them to write twice as much as they need to 
				//Some will be "burnt up" in the bedding in time
				processes.push_back(new WriterProcess<int>(pr.second,0, COMMS_EACH * 2));
			}
		}
		
		const unsigned READS = COMMS_EACH * WRITERS_PER_CHANNEL * CHANNELS;
		
		processes.push_back(new StressedAlter(ins,result.writer(),READS));
		
		ScopedForking forking;
		
		if (KERNEL_THREADS)
		{
			forking.fork(InParallel(processes.begin(),processes.end()));
		}
		else
		{	
			//Fork them as one thread, but not this one:
			forking.fork(InParallelOneThread(processes.begin(),processes.end()));
		}
		Time t;
		
		result.reader() >> t;
		
		return TestResultPass("Stressed Alter Test, " 
			+ lexical_cast<string>(CHANNELS) + " channels * "
			+ lexical_cast<string>(WRITERS_PER_CHANNEL) + " writers per channel, took: "
			+ lexical_cast<string>(1000000.0 * (GetSeconds(&t) / static_cast<double>(READS))) + " microseconds per single fairSelect() using "
			+ std::string(KERNEL_THREADS ? " threads" : " no threads")
		);
	}

	std::list<TestResult (*)()> tests()
	{
		us = currentProcess();
		return list_of<TestResult (*) ()>
			(testNormal0) (testNormal1) (testNormal2) (testNormal3) (testNormal4)
			(testBuf0) (testBuf1) (testBuf2) (testBuf3) (testBuf4)			
			
			(testRepGuard0) (testRepGuard1)
		;
	}
	
	std::list<TestResult (*)()> perfTests()
	{
		us = currentProcess();		
		return list_of<TestResult (*) ()>
			( testStressedAlt<10,10,1000,true, StandardChannelFactory<int> > )			
			( testStressedAlt<10,100,100,true, StandardChannelFactory<int> > )
			( testStressedAlt<100,10,100,true, StandardChannelFactory<int> > )
			
			( testStressedAlt<10,10,1000,false, StandardChannelFactory<int> > )
			( testStressedAlt<10,100,100,false, StandardChannelFactory<int> > )
			( testStressedAlt<100,10,100,false, StandardChannelFactory<int> > )
			( testStressedAlt<50,50,100,false, StandardChannelFactory<int> > )			
		;
	}	
	
	inline virtual ~AltChannelTest()
	{
	}
};

ProcessPtr AltChannelTest::us;

Test* GetAltChannelTest()
{
	return new AltChannelTest;
}
