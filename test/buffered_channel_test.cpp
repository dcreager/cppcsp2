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

class TestBuffer : public ChannelBuffer<int>
	{
	
	public:
		inline TestBuffer() {buffer = this;reset();}
		
		inline void reset() {putted = getted = beganExtGet = endedExtGet = cleared = false;}
	
		typedef csp::ChannelBufferFactoryImpl<int,TestBuffer> Factory;
		
		static TestBuffer* buffer;
		
		int data;
		
		//Gloriously bad English:
		bool putted;
		bool getted;
		bool beganExtGet;
		bool endedExtGet;
		bool cleared;
		
		bool inputPoss;
		bool outputPoss;
		
		
		virtual void put(const int* source) {data = *source;putted = true;}
		virtual void get(int* dest) {*dest = data;getted = true;}
		virtual void beginExtGet(int* dest) {beganExtGet = true;*dest = data;}
		virtual void endExtGet() {endedExtGet = true;}
		virtual void clear() {cleared = true;}
		
		void setEmpty() {inputPoss = false;outputPoss = true;}
		void setFull() {inputPoss = true;outputPoss = false;}
		void setNormal() {inputPoss = true;outputPoss = true;}
		
		virtual bool inputWouldSucceed() {return inputPoss;}
		virtual bool outputWouldSucceed(const int*) {return outputPoss;}
	};
	
TestBuffer* TestBuffer::buffer;


class BufferedChannelTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static ProcessPtr us;	
	
	
	static void checkReadMutex(BufferedOne2OneChannel<int>&,bool,int) {}
	static void checkReadMutex(BufferedOne2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.readerMutex.isClaimed(),"Read mutex (is/not) claimed",line);}
	static void checkReadMutex(BufferedAny2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.readerMutex.isClaimed(),"Read mutex (is/not) claimed",line);}
	static void checkWriteMutex(BufferedOne2OneChannel<int>&,bool,int) {}
	static void checkWriteMutex(BufferedAny2OneChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.writerMutex.isClaimed(),"Write mutex (is/not) claimed",line);}
	static void checkWriteMutex(BufferedAny2AnyChannel<int>& c,bool claimed,int line) {ASSERTEQ(claimed,c.writerMutex.isClaimed(),"Write mutex (is/not) claimed",line);}
	
	template <typename CHANNEL>
	inline static void checkReadWriteMutex(CHANNEL& c,bool readClaimed,bool writeClaimed,int line)
	{
		checkReadMutex(c,readClaimed,line);
		checkWriteMutex(c,writeClaimed,line);
	}
	
	
	
	/**
	*	Tests that the reader blocks when the channel is empty
	*/
	template <typename CHANNEL>
	static TestResult test0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory  factory;
		CHANNEL c (factory);
		
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
			
			TestBuffer::buffer->setEmpty();
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_reader->t),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			checkReadWriteMutex(c,true,false,__LINE__);
			
			//So that the put will happen:
			TestBuffer::buffer->setNormal();
		
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We release them from the channel
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				c.writer() << 42;
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
															
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected (B)",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected (B)",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->getted,"Channel buffer has not had get() called (B)",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer has not had put() called (B)",__LINE__);
			
			checkReadWriteMutex(c,true,false,__LINE__);
			CPPCSP_Yield(); //reader can clean up
			checkReadWriteMutex(c,false,false,__LINE__);
		
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() +  " Test Reader Empty");
	}
	
	/**
	*	Checks that the reader does not block when there is some data in the channel (buffer neither full nor empty)
	*/
	template <typename CHANNEL>
	static TestResult test1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory  factory;
		CHANNEL c(factory);
		
		{
			ScopedForking forking;
			
	        ReaderProcess<int>* _reader = new ReaderProcess<int>(c.reader(),1);
    		ProcessPtr reader(getProcessPtr(_reader));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They read from the channel without blocking, then finish
			;
			
			TestBuffer::buffer->setNormal();
			
			checkReadWriteMutex(c,false,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->getted,"Channel buffer was not read from",__LINE__);
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Test Reader Half-Full");
	}
	
	/**
	*	Checks that the reader does not block when the buffer is full
	*/
	template <typename CHANNEL>
	static TestResult test2()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory  factory;
		CHANNEL c( factory );
		
		{
			ScopedForking forking;
			
	        ReaderProcess<int>* _reader = new ReaderProcess<int>(c.reader(),1);
    		ProcessPtr reader(getProcessPtr(_reader));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They read from the channel without blocking, then finish
			;
			
			checkReadWriteMutex(c,false,false,__LINE__);
			
			TestBuffer::buffer->setFull();
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->getted,"Channel buffer was not read from",__LINE__);
		
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Test Reader Full");
	}
	
	/**
	*	Tests that the writer does not block when the buffer is empty
	*/
	template <typename CHANNEL>
	static TestResult test3()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory factory;
		CHANNEL c ( factory );
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),7,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They write the channel without blocking, then finish
			;
			
			TestBuffer::buffer->setEmpty();
			checkReadWriteMutex(c,false,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was not written to",__LINE__);
			ASSERTEQ(7,TestBuffer::buffer->data,"Correct value was not written",__LINE__);
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Test Writer Empty");
	}
	
	/**
	*	Tests that the writer does not block when the buffer is half-full (neither empty nor full)
	*/
	template <typename CHANNEL>
	static TestResult test4()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory factory;
		CHANNEL c (factory);
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),7,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They write to the channel without blocking, then finish
			;
			
			TestBuffer::buffer->setNormal();
			checkReadWriteMutex(c,false,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was not written to",__LINE__);
			ASSERTEQ(7,TestBuffer::buffer->data,"Correct value was not written",__LINE__);
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Test Writer Half-Full");
	}
	
	/**
	*	Tests that the writer blocks when the buffer is full
	*/
	template <typename CHANNEL>
	static TestResult test5()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory factory;
		CHANNEL c( factory );
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),7,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			TestBuffer::buffer->setFull();
			TestBuffer::buffer->data = 9;
			checkReadWriteMutex(c,false,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was written to",__LINE__);
			
			checkReadWriteMutex(c,false,true,__LINE__);
			
			//To let the put work:
			TestBuffer::buffer->setNormal();
		
			EventList expB;
			
			expB = tuple_list_of
				(us,writer,writer) //We release them from the channel
			;
			
			int n;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				
				c.reader() >> n;
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected (B)",__LINE__);			
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected (B)",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->getted,"Channel buffer has not had get() called (B)",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer has not had put() called (B)",__LINE__);
			ASSERTEQ(9,n,"Incorrect item of data was read",__LINE__);
			ASSERTEQ(7,TestBuffer::buffer->data,"Data was not put into the buffer correctly",__LINE__);
			
			checkReadWriteMutex(c,false,true,__LINE__);
			CPPCSP_Yield();
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Test Writer Full");
	}	
	
	/**
	*	Tests that the reader blocks on an extended input when the channel and buffer is empty (i.e. the reader arriving first, to an empty buffer)
	*/
	template <typename CHANNEL>
	static TestResult testExt0()
	{
		TestBuffer::Factory  factory;
		CHANNEL c (factory);
		One2OneChannel<int> d;
	
		BEGIN_TEST()
		
		SetUp setup;				
		
		{
			ScopedForking forking;
			
	        ExtId<int>* _reader = new ExtId<int>(c.reader(),d.writer());
    		ProcessPtr reader(getProcessPtr(_reader));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the channel
			;
			
			TestBuffer::buffer->setEmpty();
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
		
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We release them from the channel				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on channel d
			;
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				c.writer() << 42;
				
				CPPCSP_Yield();	
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			TestBuffer::buffer->reset();
		
			EventList expC;
			
			expC = tuple_list_of				
				(us,reader,reader) //We release them from channel d
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield again
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They see the poison and finish
			;
			
			EventList actC;
			{			
				RecordEvents _(&actC);			
				
				int n;
				d.reader() >> n;
				
				c.writer().poison();				
				CPPCSP_Yield();				
			}
				
			
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part C",__LINE__);
									
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
		}
		
		END_TEST_C(ChannelName<CHANNEL>::Name() + " Test Extended Reader Empty",c.writer().poison();d.reader().poison());
	}		
	
	/**
	*	Tests that the reader does not block on an extended input when the buffer is half-full
	*/
	template <typename CHANNEL>
	static TestResult testExt1()
	{
		TestBuffer::Factory  factory;
		CHANNEL c (factory);
		One2OneChannel<int> d;
	
		BEGIN_TEST()
		
		SetUp setup;				
		
		{
			ScopedForking forking;
			
	        ExtId<int>* _reader = new ExtId<int>(c.reader(),d.writer());
    		ProcessPtr reader(getProcessPtr(_reader));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the d channel
			;
			
			TestBuffer::buffer->setNormal();
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			ASSERTEQ(reader,d.waiting,"Channel data (d.waiting) not as expected",__LINE__);
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,d,src),"Channel data (d.src) not as expected",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
		
		
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We release them from the channel				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on channel c again
			;
			
			TestBuffer::buffer->setEmpty();
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				int n;
				d.reader() >> n;
				
				CPPCSP_Yield();	
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			c.writer().poison();
			d.reader().poison();
		}
		
		END_TEST_C(ChannelName<CHANNEL>::Name() + " Test Extended Reader Half-Full",c.writer().poison();d.reader().poison());
	}
	
	/**
	*	Tests extended read from a full buffer (with a writer that then blocks)
	*/
	template <typename CHANNEL>
	static TestResult testExt2()
	{
		TestBuffer::Factory  factory;
		CHANNEL c (factory);
		One2OneChannel<int> d;
	
		BEGIN_TEST()
		
		SetUp setup;				
		
		{
			ScopedForking forking;
			
	        ExtId<int>* _reader = new ExtId<int>(c.reader(),d.writer());
    		ProcessPtr reader(getProcessPtr(_reader));
    		WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),7,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) (us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the d channel
				(writer,NullProcessPtr,NullProcessPtr) //They block on the c channel
			;
			
			TestBuffer::buffer->setFull();
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			ASSERTEQ(reader,d.waiting,"Channel data (d.waiting) not as expected",__LINE__);
			ASSERTEQ(&(_reader->t),ACCESS(One2OneChannel<int>,d,src),"Channel data (d.src) not as expected",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,true,__LINE__);
		
		
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We release them from the channel				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,writer,writer) //They free the writer, having performed its input
				(reader,NullProcessPtr,NullProcessPtr) //They block on channel c again
			;
			
			//Make the reader block (a bit hacky):
			TestBuffer::buffer->setEmpty();
			
			EventList actB;
			{
				RecordEvents _(&actB);
				
				int n;
				d.reader() >> n;
				
				CPPCSP_Yield();	
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,true,__LINE__);
			
			//Let the writer finish (they only write once):
			
			CPPCSP_Yield();	
			
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			c.writer().poison();
			d.reader().poison();
		}
		
		END_TEST_C(ChannelName<CHANNEL>::Name() + " Test Extended Reader Full, Reader First",c.writer().poison();d.reader().poison());
	}
	
	/**
	*	Tests extended read from a full buffer (with a writer already waiting)
	*/
	template <typename CHANNEL>
	static TestResult testExt3()
	{
		TestBuffer::Factory  factory;
		CHANNEL c (factory);
		One2OneChannel<int> d;
	
		BEGIN_TEST()
		
		SetUp setup;				
		
		{
			ScopedForking forking;
			
	        ExtId<int>* _reader = new ExtId<int>(c.reader(),d.writer());
    		ProcessPtr reader(getProcessPtr(_reader));
    		WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),7,1);
    		ProcessPtr writer(getProcessPtr(_writer));
                						
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(writer,NullProcessPtr,NullProcessPtr) //They block on the c channel				
				
			;
			
			TestBuffer::buffer->setFull();
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			EventList actA;
			{
				RecordEvents _(&actA);
								
				forking.forkInThisThread(_writer);
				
				CPPCSP_Yield();
			}
			
			ASSERTEQ(expA,actA,"Channel events not as expected in part A",__LINE__);
						
			ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
									
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,true,__LINE__);		
		
			EventList expB;
			
			expB = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield				
				(reader,NullProcessPtr,NullProcessPtr) //They block on channel d
			;						
			
			EventList actB;
			{
				RecordEvents _(&actB);								
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();	
			}
			
			ASSERTEQ(expB,actB,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,true,__LINE__);
			
			
			EventList expC;
			
			expC = tuple_list_of
				(us,reader,reader) //We release them from the channel				
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,writer,writer) //They free the writer, having performed its input
				(reader,NullProcessPtr,NullProcessPtr) //They block on channel c again
			;
			
			//Make the reader block (a bit hacky):
			TestBuffer::buffer->setEmpty();
			
			EventList actC;
			{
				RecordEvents _(&actC);
				
				int n;
				d.reader() >> n;
				
				CPPCSP_Yield();	
			}
			
			ASSERTEQ(expC,actC,"Channel events not as expected in part B",__LINE__);
						
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,true,__LINE__);
						
			
			//Let the writer finish (they only write once):
			
			CPPCSP_Yield();	
			
			ASSERTEQ(reader,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->putted,"Channel buffer was read from",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->beganExtGet,"Channel buffer began an extended input",__LINE__);
			ASSERTEQ(true,TestBuffer::buffer->endedExtGet,"Channel buffer ended an extended input",__LINE__);
			
			checkReadMutex(c,true,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			
			c.writer().poison();
			d.reader().poison();
		}
		
		END_TEST_C(ChannelName<CHANNEL>::Name() + " Test Extended Reader Full, Writer First",c.writer().poison();d.reader().poison());
	}

	
	/**
	*	Tests that poison released by the reader always reaches the writer regardless of the buffer state
	*/

	template <typename CHANNEL>
	static TestResult testPoison0()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		TestBuffer::Factory  factory;
		
		//Poisoned before writer arrives:
		for (int i = 0;i < 2;i++)
		{			
			CHANNEL c(factory);
			switch (i)
			{
				case 0:TestBuffer::buffer->setEmpty();break;
				case 1:TestBuffer::buffer->setNormal();break;
				case 2:TestBuffer::buffer->setFull();break;
			}
		
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
							
			c.reader().poison();
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was touched",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was touched",__LINE__);
			
			try
			{
				c.writer() << 6;
				ASSERTL(false,"Expected poison but found none",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			checkReadMutex(c,false,__LINE__);
			checkWriteMutex(c,false,__LINE__);
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
			ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was touched",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was touched",__LINE__);
		}
		
		
		CHANNEL d(factory);
		//The writer will only block when the buffer is full:
		TestBuffer::buffer->setFull();
		
		checkReadMutex(d,false,__LINE__);
		checkWriteMutex(d,false,__LINE__);
		ASSERTEQ(NullProcessPtr,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
		ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,d,dest),"Channel data (dest) not as expected",__LINE__);
		ASSERTEQ(false,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			
		ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was touched",__LINE__);
		ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was touched",__LINE__);
		
		{
			ScopedForking forking;
			
	        WriterProcess<int>* _writer = new WriterProcess<int>(d.writer(),7,2); //it should do two outputs - if it didn't see the poison
    		ProcessPtr writer(getProcessPtr(_writer));
		
			forking.forkInThisThread(_writer);
		
			CPPCSP_Yield();
			
			checkReadMutex(d,false,__LINE__);
			checkWriteMutex(d,true,__LINE__);
			ASSERTEQ(writer,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was touched",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was touched",__LINE__);
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) //We release them from the channel
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				d.reader().poison();
			}
			
			ASSERTEQ(expA,actA,"Events not as expected",__LINE__);						
			
			//Let the writer clean up:
			CPPCSP_Yield();
			
			checkReadMutex(d,false,__LINE__);
			checkWriteMutex(d,false,__LINE__);
			ASSERTEQ(NullProcessPtr,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,d,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			
			ASSERTEQ(false,TestBuffer::buffer->getted,"Channel buffer was touched",__LINE__);
			ASSERTEQ(false,TestBuffer::buffer->putted,"Channel buffer was touched",__LINE__);
			
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Poison Test (Reader Poisoning For Writer)")
	}	
	
	/**
	*	Tests that poison released by the writer does not reach the reader until the appropriate time.
	*/
	template <typename CHANNEL>
	static TestResult testPoison1()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		FIFOBuffer<int>::Factory factory(2);
		
		CHANNEL c0(factory),c2(factory),d(factory);
				
		{
			ScopedForking forking;
			int n;
			
			//Channel poisoned before reader ever arrives:
			
			c2.writer() << 20;
			c2.writer() << 21;
						
			ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			ASSERTEQ(true,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(false,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
			c2.writer().poison();
			
			ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			ASSERTEQ(true,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(false,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
		
			try
			{				
				c2.reader() >> n;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison not expected!",__LINE__);
			}
			
			ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			ASSERTEQ(true,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(true,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			ASSERTEQ(20,n,"Channel data not as expected",__LINE__);
			
			try
			{				
				c2.reader() >> n;
			}
			catch (PoisonException)
			{
				ASSERTL(false,"Poison not expected!",__LINE__);
			}
			
			ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			ASSERTEQ(false,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);			
			ASSERTEQ(true,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			ASSERTEQ(21,n,"Channel data not as expected",__LINE__);
			
			try
			{				
				c2.reader() >> n;
				ASSERTL(false,"Poison expected!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		
			ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c2,false,false,__LINE__);
			ASSERTEQ(false,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(true,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);			
			
			ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c0,false,false,__LINE__);
			ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
			c0.writer().poison();
			
			ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c0,false,false,__LINE__);
			ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
			try
			{
				int n;
				c0.reader() >> n;
				ASSERTL(false,"Poison expected!",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(true,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
			checkReadWriteMutex(c0,false,false,__LINE__);
			ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
			ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
			//Now test for when the writer poisons while the reader is waiting - which can only happen on an empty channel:
			
			ReaderProcess<int>* _reader = new ReaderProcess<int>(d.reader(),2);
    		ProcessPtr reader(getProcessPtr(_reader));
			
			EventList expA;
			
			expA = tuple_list_of
				(us,reader,reader) //We fork them
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(reader,NullProcessPtr,NullProcessPtr) //They block on the empty channel
				(us,reader,reader) //We poison the channel, waking them up
				(us,us,us) (us,NullProcessPtr,NullProcessPtr) //We yield
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //They finish because of the poison
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_reader);
				
				CPPCSP_Yield();
				
				ASSERTEQ(reader,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(&(_reader->t),ACCESS(BufferedOne2OneChannel<int>,d,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(false,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(d,true,false,__LINE__);
				ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				d.writer().poison();
				
				ASSERTEQ(NullProcessPtr,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(&(_reader->t),ACCESS(BufferedOne2OneChannel<int>,d,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(d,true,false,__LINE__);
				ASSERTEQ(false,d.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,d.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				CPPCSP_Yield();
				
				ASSERTEQ(NullProcessPtr,d.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(&(_reader->t),ACCESS(BufferedOne2OneChannel<int>,d,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(true,d.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(d,false,false,__LINE__);
				ASSERTEQ(false,d.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,d.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			}
			
			ASSERTEQ(expA,actA,"Events not as expected",__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Poison Test (Writer Poisoning For Reader)")
	}		
	
	/**
	*	Tests that poison released by the writer does not reach the reader until the appropriate time.
	*/
	template <typename CHANNEL>
	static TestResult testPoison1A()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		FIFOBuffer<int>::Factory factory(2);
		
		CHANNEL c(factory);
				
		{
			ScopedForking forking;
						
			//Now test for when the writer poisons while the reader is waiting - which can only happen on an empty channel:
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),8,2);
    		ProcessPtr writer(getProcessPtr(_writer));
    		CSProcessPtr _poisoner = new ChannelPoisoner< Chanout<int> >(c.writer());
			ProcessPtr poisoner = getProcessPtr(_poisoner);
			
			EventList expA;
			
			expA = tuple_list_of
				(us,writer,writer) (us,poisoner,poisoner) //We fork them
				(us,NullProcessPtr,NullProcessPtr) //We block on the empty channel
				(writer,us,us) //They free us
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //Writer finishes
				(NullProcessPtr,NullProcessPtr,NullProcessPtr) //Poisoner finishes				
			;
			
			EventList actA;
			{
				RecordEvents _(&actA);
				
				forking.forkInThisThread(_writer);
				forking.forkInThisThread(_poisoner);
				
				//Should not see poison because the writer has written two items of data, so we should
				//only see the poison after two successful reads!
				
				int n;
				
				ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c,false,false,__LINE__);
				ASSERTEQ(false,c.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				try
				{
					c.reader() >> n;
				}
				catch (PoisonException e)
				{
					ASSERTL(false,"Poisoned when we should NOT have been",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c,false,false,__LINE__);
				ASSERTEQ(true,c.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				try
				{
					c.reader() >> n;
				}
				catch (PoisonException e)
				{
					ASSERTL(false,"Poisoned when we should NOT have been",__LINE__);
				}
				
				ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c,false,false,__LINE__);
				ASSERTEQ(false,c.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				try
				{
					c.reader() >> n;
					ASSERTL(false,"NOT poisoned when we SHOULD have been",__LINE__);
				}
				catch (PoisonException e)
				{					
				}
				
				ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,dest),"Channel data (dest) not as expected",__LINE__);
				ASSERTEQ(true,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c,false,false,__LINE__);
				ASSERTEQ(false,c.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
								
			}
			
			ASSERTEQ(expA,actA,"Events not as expected",__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Poison Test (Writer Poisoning For Reader, second test)")
	}
	
	/**
	*	Tests that writer-poison is always seen by the writer
	*/
	template <typename CHANNEL>
	static TestResult testPoison2()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		FIFOBuffer<int>::Factory factory(2);
		
		CHANNEL c0(factory),c1(factory),c2(factory);
				
		{			
			//Channel poisoned before reader ever arrives:
			
			c2.writer() << 20;
			c2.writer() << 21;
			c1.writer() << 10;
			
			for (int i = 0;i < 2;i++)
			{
				//Once before poison, once after poison:
				
				ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c2,false,false,__LINE__);
				ASSERTEQ(true,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(false,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c1,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c1,false,false,__LINE__);
				ASSERTEQ(true,c1.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c1.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c0,false,false,__LINE__);
				ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
				if (i == 0)
				{			
					c0.writer().poison();
					c1.writer().poison();
					c2.writer().poison();
				}
			}
			
			try
			{
				c0.writer() << 0;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			try
			{
				c1.writer() << 11;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			try
			{
				c2.writer() << 22;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Poison Test (Writer Poisoning For Writer)")
	}
	
	/**
	*	Tests that reader-poison is always seen by the reader
	*/
	template <typename CHANNEL>
	static TestResult testPoison3()
	{
		BEGIN_TEST()
		
		SetUp setup;
		
		FIFOBuffer<int>::Factory factory(2);
		
		CHANNEL c0(factory),c1(factory),c2(factory);
				
		{			
			//Channel poisoned before reader ever arrives:
			
			c2.writer() << 20;
			c2.writer() << 21;
			c1.writer() << 10;
			
			for (int i = 0;i < 2;i++)
			{
				//Once before poison, once after poison:
				
				ASSERTEQ(NullProcessPtr,c2.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c2,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c2.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c2,false,false,__LINE__);
				//true,false before poison; false,true after
				ASSERTEQ(i == 0,c2.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(i != 0,c2.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				ASSERTEQ(NullProcessPtr,c1.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c1,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c1.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c1,false,false,__LINE__);
				//true before poison, false after
				ASSERTEQ(i == 0,c1.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c1.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
				
				ASSERTEQ(NullProcessPtr,c0.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
				ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c0,src),"Channel data (src) not as expected",__LINE__);
				ASSERTEQ(i == 1,c0.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);
				checkReadWriteMutex(c0,false,false,__LINE__);
				ASSERTEQ(false,c0.buffer->inputWouldSucceed(),"Buffer not as expected",__LINE__);
				ASSERTEQ(true,c0.buffer->outputWouldSucceed(NULL),"Buffer not as expected",__LINE__);
			
				if (i == 0)
				{			
					c0.reader().poison();
					c1.reader().poison();
					c2.reader().poison();
				}
			}
			
			int n;
			
			try
			{
				c0.reader() >> n;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			try
			{
				c1.reader() >> n;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
			
			try
			{
				c2.reader() >> n;
				ASSERTL(false,"Should have noticed poison!",__LINE__);
			}
			catch (PoisonException)
			{
			}
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Poison Test (Reader Poisoning For Reader)")
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
		FIFOBuffer<int>::Factory factory(2);
		CHANNEL c(factory);
		
		{
			ScopedForking forking;
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,3);
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
						
			ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
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
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
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
						
			ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			checkReadWriteMutex(c,false,false,__LINE__);
		}
		
		END_TEST(ChannelName<CHANNEL>::Name() + " Buffered Extended Poison Mid-way Test");
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
		
		FIFOBuffer<int>::Factory factory(2);
		CHANNEL c(factory);
		
		{
			ScopedForking forking;
			
			WriterProcess<int>* _writer = new WriterProcess<int>(c.writer(),0,3);
    		ProcessPtr writer(getProcessPtr(_writer));
    		
    		forking.forkInThisThread(_writer);
    		
    		//Get them blocked on the channel:
    		CPPCSP_Yield();
    		
    		ASSERTEQ(writer,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);
			ASSERTEQ(&(_writer->t),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
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
    		
    		ASSERTEQ(NullProcessPtr,c.waitingProcess,"Channel data (waiting) not as expected",__LINE__);			
    		ASSERTEQ(static_cast<int*>(NULL),ACCESS(BufferedOne2OneChannel<int>,c,src),"Channel data (src) not as expected",__LINE__);
			ASSERTEQ(false,c.isPoisoned,"Channel data (isPoisoned) not as expected",__LINE__);			
			checkReadWriteMutex(c,false,true,__LINE__);
			CPPCSP_Yield();
			checkReadWriteMutex(c,false,false,__LINE__);
			
    	}
    	
    	END_TEST(ChannelName<CHANNEL>::Name() + " Buffered Extended Exception Mid-way Test");
	}
	
	/**
	*	Tests that the FIFOBuffer class works properly
	*/
	static TestResult testFIFO()
	{
		BEGIN_TEST()
		
		int n;
		FIFOBuffer<int> buffer(3);
		
		//Should start empty:
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer did not start empty",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer did not start empty",__LINE__);
		
		//Now put some things in it:
		
		n = 1;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 2;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 3;
		buffer.put(&n);
		
		ASSERTEQ(false,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now it's full.  Let's empty it again:
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(1,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(2,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(3,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now test extended input.
		
		n = 4;
		buffer.put(&n);
		
		n = 99;
		buffer.beginExtGet(&n);
		
		ASSERTEQ(4,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);		
		
		//Two more items should fill it:
		
		n = 5;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		n = 6;
		buffer.put(&n);
		ASSERTEQ(false,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		//Now this should get it back down:
				
		buffer.endExtGet();
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now just check that the other items come out properly:

		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(5,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(6,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		END_TEST("FIFO Buffer Test");
	}
	
	static TestResult testOverwrite()
	{
		BEGIN_TEST()
		
		int n;
		OverwritingBuffer<int> buffer(3);
		
		//Should start empty:
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer did not start empty",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer did not start empty",__LINE__);
		
		//Now put some things in it - should behave like a FIFO unless we exceed the capacity
		
		n = 1;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
	
		n = 2;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 3;
		buffer.put(&n);
		
		ASSERTEQ(true /*unlike FIFO*/,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now it's full.  Let's empty it again:
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(1,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(2,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(3,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now we need to test the overwriting aspect:
		
		n = 1;
		buffer.put(&n);	
		n = 2;
		buffer.put(&n);		
		n = 3;
		buffer.put(&n);		
		//Now overwrite 1 with 4:
		n = 4;
		buffer.put(&n);
				
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(2,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(3,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(4,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Do this sort of thing a couple more times:
		
		n = 5;
		buffer.put(&n);	
		n = 6;
		buffer.put(&n);		
		n = 7;
		buffer.put(&n);		
		//Now overwrite 5 with 8:
		n = 8;
		buffer.put(&n);
		//Now overwrite 6 with 9:
		n = 9;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(7,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(8,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(9,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 10;
		buffer.put(&n);	
		n = 11;
		buffer.put(&n);		
		n = 12;
		buffer.put(&n);				
		n = 13;
		buffer.put(&n);		
		n = 14;
		buffer.put(&n);
		n = 15;
		buffer.put(&n);
		n = 16;
		buffer.put(&n);
		n = 17;
		buffer.put(&n);
		n = 18;
		buffer.put(&n);
		n = 19;
		buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(17,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(18,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(19,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now test extended input - this is going to get confusing!

		//Test without overwriting:
		
		n = 20;
		buffer.put(&n);
		
		n = 99;
		buffer.beginExtGet(&n);
		
		ASSERTEQ(20,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);		
		
		//Two more items should fill it:
		
		n = 21;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		n = 22;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		//Now this should get it back down:
				
		buffer.endExtGet();
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now just check that the other items come out properly:

		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(21,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(22,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);				
		
		//Test with overwriting:
		
		n = 23;
		buffer.put(&n);
		
		n = 99;
		buffer.beginExtGet(&n);
		
		ASSERTEQ(23,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);		
		
		//Two more items should fill it:
		
		n = 24;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		n = 25;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		n = 26;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		n = 27;
		buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		
		//Currently in the buffer should be 25,26,27.
				
		buffer.endExtGet();
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		//Now just check that the other items come out properly:

		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(25,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(26,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		n = 99;
		buffer.get(&n);
		
		ASSERTEQ(27,n,"Buffer did not return the right value",__LINE__);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);		
		
		END_TEST("OverwritingBuffer Test");
	}
	
	static TestResult testInfinite()
	{
		BEGIN_TEST()
		
		int n;
		InfiniteFIFOBuffer<int> buffer;
		
		//Should start empty:
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer did not start empty",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer did not start empty",__LINE__);
		
		n = 1;buffer.put(&n);
		
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		n = 2;buffer.put(&n);
		n = 3;buffer.put(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		
		buffer.get(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(1,n,"Data value not as expected",__LINE__);
		
		buffer.get(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(true,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(2,n,"Data value not as expected",__LINE__);
		
		buffer.get(&n);
		ASSERTEQ(true,buffer.outputWouldSucceed(&n),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(false,buffer.inputWouldSucceed(),"Buffer not behaving properly",__LINE__);
		ASSERTEQ(3,n,"Data value not as expected",__LINE__);
		
		END_TEST("InfiniteFIFOBuffer Test");
	}			
	
	
	std::list<TestResult (*)()> tests()
	{
		#define BUFFERED_CHANNELS (BufferedOne2OneChannel<int>)(BufferedOne2AnyChannel<int>)(BufferedAny2OneChannel<int>)(BufferedAny2AnyChannel<int>)
	
		us = currentProcess();
		return list_of<TestResult (*) ()>
			TEST_FOR_EACH(test0, BUFFERED_CHANNELS )
			TEST_FOR_EACH(test1, BUFFERED_CHANNELS )
			TEST_FOR_EACH(test2, BUFFERED_CHANNELS )
			TEST_FOR_EACH(test3, BUFFERED_CHANNELS )
			TEST_FOR_EACH(test4, BUFFERED_CHANNELS )
			TEST_FOR_EACH(test5, BUFFERED_CHANNELS )						
						
			TEST_FOR_EACH(testExt0, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testExt1, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testExt2, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testExt3, BUFFERED_CHANNELS )
			
			TEST_FOR_EACH(testPoison0, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testPoison1, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testPoison1A, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testPoison2, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testPoison3, BUFFERED_CHANNELS )
			
			TEST_FOR_EACH(testExtendedPoison, BUFFERED_CHANNELS )
			TEST_FOR_EACH(testExtException, BUFFERED_CHANNELS )
						
			(testFIFO) (testOverwrite) (testInfinite)
		;
	}
	
	std::list<TestResult (*)()> perfTests()
	{
		us = currentProcess();
		return std::list<TestResult (*)()>();
	}	
	
	inline virtual ~BufferedChannelTest()
	{
	}
};

ProcessPtr BufferedChannelTest::us;

Test* GetBufferedChannelTest()
{
	return new BufferedChannelTest;
}
