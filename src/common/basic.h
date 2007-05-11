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

#include <list>

class AltChannelTest;
class ChannelTest;
class BufferedChannelTest;

namespace csp
{
namespace common
{

	/**
	*	A process that does nothing.
	*
	*	This process is only really useful where you need to supply a process for some reason, but you want to supply an empty process.
	*
	*	Named SkipProcess (rather than simply Skip) to distinguish from SkipGuard.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*/
	class SkipProcess : public CSProcess
	{
	protected:
		void run()
		{
			//Skip!
		}
	public:
		SkipProcess() : CSProcess(65536) {}
	};

	/**
	*	A process that forever forwards an item of data to another channel.
	*
	*	This process forever reads a value from its input channel, then writes it on its output channel.
	*
	*	It can be used for various purposes, including connecting two channels together.  Be aware that two channels
	*	connected by an Id process is not semantically equivalent to a single channel, because the Id process inherently 
	*	introduces buffering (the writer of the first channel can complete the communication without the reader of the second
	*	channel having done anything, because of the Id process in the middle).  To avoid this behaviour, look at the ExtId
	*	process.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.	
	*/
	template <typename DATA_TYPE>
	class Id : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;
		DATA_TYPE t;
	protected:
		void run()
		{
			try
			{
				while (true)
				{
					in >> t;
					out << t;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison(); 
			}
		}
	public:
		/**
		*	Constructs the Id process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*/
		inline Id(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_in),out(_out)
		{
		}
	};

	/**
	*	A process that forever performs an extended input from a channel, sending the data out on another channel as its extended action.
	*
	*	Unlike Id, this process does not introduce buffering while connecting two channels.  Two unbuffered channels connected
	*	via an ExtId are semantically equivalent to a single unbuffered channel.  However, with buffered channels the effect
	*	may be different, especially if they are overwriting buffers!
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class ExtId : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;		
		DATA_TYPE t;		
	protected:
		void run()
		{			
			try
			{
				while (true)
				{
					{ 
						ScopedExtInput<DATA_TYPE> extInput(in,&t);						
						out << t;
					}
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}		
		}
	public:
		/**
		*	Constructs the ExtId process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*/
		inline ExtId(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_in),out(_out)
		{
		}		
		
		#ifndef CPPCSP_DOXYGEN
		//For testing:
		friend class ::BufferedChannelTest;
		#endif
	};
	
	
	/**
	*	A process that forever performs an extended input from a channel, and for its extended action: syncs on a barrier and then sends the data out on another channel.
	*
	*	Just to be clear: both the barrier sync and the channel output are part of the extended action, and happen in the order: sync, write.
	*
	*	This can be useful when you want to have an invisible process connecting two channels, but only allow
	*	data to proceed along a channel when a barrier sync occurs, or to look at it from another perspective, to only
	*	allow a barrier to sync when data passes along the channel.
	*
	*	For reasons similar to the BarrierSyncer process, you will probably want to give this process a pre-enrolled
	*	end of the barrier.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.
	*/	
	template <typename DATA_TYPE>
	class ExtSyncId : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;
		Mobile<BarrierEnd> end;
		DATA_TYPE t;		
	protected:
		void run()
		{
			end->enroll();
			
			try
			{
				while (true)
				{
					{ 
						ScopedExtInput<DATA_TYPE> extInput(in,&t);
							
						end->sync();
						
						out << t;
					}
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
			
			end->resign();
		}
	public:
		/**
		*	Constructs the ExtSyncId process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*	@param _end The barrier end
		*/
		inline ExtSyncId(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out, const Mobile<BarrierEnd>& _end)
			:	CSProcess(65536),in(_in),out(_out),end(_end)
		{
		}
		
		#ifndef CPPCSP_DOXYGEN
		//For testing:
		friend class ::ChannelTest;		
		#endif
	};	
	
	/**
	*	Writes a given value to a channel a specified number of times.
	*
	*	When the process has completed the specified number of writes, it simply finishes.  It does not poison the channel.
	*	If you want to poison the channel afterwards, consider using something like this:
	*	@code
			Run( InSequenceOneThread(new WriterProcess<int>(out,21,7)) (new ChannelPoisoner(out)) );
		@endcode
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class WriterProcess : public CSProcess
	{
	private:
		Chanout<DATA_TYPE> out;
		DATA_TYPE t;
		int times;
	protected:
		void run()
		{
			try
			{
				for (int i = 0;i < times;i++)
				{
					out << t;
				}
			}
			catch (PoisonException&)
			{
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process.
		*
		*	@param _out The channel to write to
		*	@param value The value to write to the channel
		*	@param _times The number of times to write the value to the channel.  Defaults to one.
		*/
		inline WriterProcess(const Chanout<DATA_TYPE>& _out, const DATA_TYPE& value, int _times = 1)
			:	CSProcess(65536),out(_out),t(value),times(_times)
		{
		}
		
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::AltChannelTest;
		friend class ::BufferedChannelTest;		
	};



	/**
	*	Reads values from a channel a specified number of times.
	*
	*	Nothing is done with the values; it is as if they vanish.  Therefore this process is only really for absorbing
	*	values you don't want from a channel.  It is also used for testing C++CSP2.
	*
	*	When the process has completed the specified number of reads, it simply finishes.  It does not poison the channel.
	*	If you want to poison the channel afterwards, consider using something like this:
	*	@code
			Run( InSequenceOneThread(new ReaderProcess<int>(in,7)) (new ChannelPoisoner(in)) );
		@endcode
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class ReaderProcess : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		DATA_TYPE t;
		int times;
	protected:
		void run()
		{
			try
			{
				for (int i = 0;i < times;i++)
				{
					in >> t;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
			}
		}
	public:
		/**
		*	Constructs the process.
		*
		*	@param _in The channel to read from		
		*	@param _times The number of times to read from the channel.  Defaults to one.
		*/
		inline explicit ReaderProcess(const Chanin<DATA_TYPE>& _in, int _times = 1)
			:	CSProcess(65536),in(_in),times(_times)
		{
		}
		
		//For testing:
		friend class ::ChannelTest;
		friend class ::BufferedChannelTest;
	};
	
	
	/** 
	*	Writes a given value (passed by pointer) to a channel once.
	*
	*	This process is very similar to WriterProcess.  There are two main differences.  WriterProcess
	*	can write to a channel any number of times, but this process only ever writes once.  The more
	*	significant difference is that WriterProcess takes the value to be communicated <b>by value</b>.
	*	In contrast, this process uses a <b>pointer</b> to a value to communicate.  Therefore this
	*	process is useful when the data to be communicated is large (or otherwise carries a high copying
	*	overhead).	
	*
	*	When the process has completed the single write, it simply finishes.  It does not poison the channel.
	*	If you want to poison the channel afterwards, consider using something like this:
	*	@code
			Run( InSequenceOneThread(new WriteOnceProcess<int>(out,21)) (new ChannelPoisoner(out)) );
		@endcode
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class WriteOnceProcess : public CSProcess
	{
	private:
		Chanout<DATA_TYPE> out;
		DATA_TYPE const * value;
	protected:
		void run()
		{
			try
			{
				out.write(value);				
			}
			catch (PoisonException&)
			{
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process.
		*
		*	@param _out The channel to write to
		*	@param _value The value to write to the channel.  This pointer must remain valid for the life time of the process		
		*/
		inline WriteOnceProcess(const Chanout<DATA_TYPE>& _out, DATA_TYPE const * _value)
			:	CSProcess(65536),out(_out),value(value)
		{
		}
	};



	/** 
	*	Reads a single value from a channel into a given (by pointer) location.
	*
	*	This process is a little similar to ReaderProcess.  ReaderProcess reads multiple values
	*	and discards the results.  ReadOnceProcess reads a singe value into a given location.  It
	*	is particularly useful for doing multiple parallel reads.
	*
	*	When the process has completed the single read, it simply finishes.  It does not poison the channel.
	*	If you want to poison the channel afterwards, consider using something like this:
	*	@code
			Run( InSequenceOneThread(new ReadOnceProcess<int>(in,&n)) (new ChannelPoisoner(in)) );
		@endcode
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class ReadOnceProcess : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		DATA_TYPE* result;		
	protected:
		void run()
		{
			try
			{
				in.read(result);				
			}
			catch (PoisonException&)
			{
				in.poison();
			}
		}
	public:
		/**
		*	Constructs the process.
		*
		*	@param _in The channel to read from		
		*	@param _result The location to read the value into
		*/
		inline explicit ReadOnceProcess(const Chanin<DATA_TYPE>& _in, DATA_TYPE* _result)
			:	CSProcess(65536),in(_in),result(_result)
		{
		}		
	};
		
	/**
	*	A process that forwards an item of data to another channel, but starts by sending a supplied initial value.
	*
	*	This process first sends out its initial value, and then repeatedly: reads a value from its input channel, then writes it on its output channel.
	*	That is, it sends out its initial value on its output channel, then behaves as Id.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class Prefix : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;
		DATA_TYPE t;
		const int times;
	protected:
		void run()
		{
			try
			{
				for (int i = 0;i < times;i++)
				{
					out << t;
				}
				while (true)
				{
					in >> t;
					out << t;					
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the Prefix process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*	@param initialValue The value to prefix to (send first on) the channel
		*	@param _times The number of times to send the prefixed value.  Defaults to one.
		*/
		inline Prefix(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out, const DATA_TYPE& initialValue, const int _times = 1)
			:	CSProcess(65536),in(_in),out(_out),t(initialValue),times(_times)
		{
		}
	};	

	/**
	*	A process that reads a value from its input channel, increments it, and writes it to the process's output channel
	*
	*	The increment is done using the prefix ++ operator.  This process is included because of its occam history, and its
	*	use in the commstime benchmark.  However, you may also find a use for it.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must support the prefix increment operator (++), and also must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class Successor : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;
		DATA_TYPE t;
	protected:
		void run()
		{
			try
			{
				while (true)
				{
					in >> t;
					++t;
					out << t;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the Successor process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*/
		inline Successor(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_in),out(_out)
		{
		}
	};	
	
	/**
	*	A process that forever forwards an item of data to two other channels -- in sequence.
	*
	*	This process forever reads a value from its input channel, then writes it on its first output channel, then
	*	writes it on its second output channel.  This ordering of outputs can lead to deadlock if the function
	*	of this process is not correctly understood.  If you wish to have parallel outputs, use the Delta process;
	*	this process is provided because it is faster than the Delta process.
	*
	*	Also be aware that this process will introduce buffering.  For a process that does not, look at using the
	*	ExtSeqDelta process.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.
	*/
	template <typename DATA_TYPE>
	class SeqDelta : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out0;
		Chanout<DATA_TYPE> out1;
		DATA_TYPE t;
	protected:
		void run()
		{
			try
			{
				while (true)
				{
					in >> t;
					out0 << t;
					out1 << t;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out0.poison();
				out1.poison();
			}
		}
	public:
		/**
		*	Constructs the SeqDelta process
		*
		*	@param _in The input channel
		*	@param _out0 The first output channel
		*	@param _out1 The second output channel
		*/
		inline SeqDelta(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out0, const Chanout<DATA_TYPE>& _out1)
			:	CSProcess(65536),in(_in),out0(_out0),out1(_out1)
		{
		}
	};	
	
	
	/**
	*	A process that sleeps for a specified amount of time.
	*
	*	This process is mainly useful to put before another in sequence (delaying the start of the later process).	
	*/
	class SleepForProcess : public CSProcess
	{
	private:
		csp::Time delay;
	protected:
		void run()
		{
			SleepFor(delay);
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _delay The amount of time to wait for, e.g. Seconds(1)
		*/
		inline SleepForProcess(const csp::Time& _delay)
			:	delay(_delay)
		{
		}
	};
	
	/**
	*	A process that sleeps until a specified time.
	*
	*	This process is mainly useful to put before another in sequence (delaying the start of the later process).	
	*/
	class SleepUntilProcess : public CSProcess
	{
	private:
		csp::Time until;
	protected:
		void run()
		{
			SleepUntil(until);
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _until The time to wait until, e.g. CurrentTime() + Seconds(1)
		*/
		inline SleepUntilProcess(const csp::Time& _until)
			:	until(_until)
		{
		}
	};	
	
	/**
	*	A process that behaves like Id -- continually reading values and then sending them on -- but with acknowledgements when the message has been sent on.
	*	
	*	The code of the process is straightforward:
		@code
			while (true)
			{
				in >> t;
				out << t;
				sentAck << true;
			}
		@endcode
	*	When poison is encountered, all channels are poisoned.  A "false" value is NOT sent; instead, the
	*	sendAck channel will be poisoned.	
	*
	*	One main use of this process -- if the buffering (of one) it inherently adds is acceptable --
	*	is to transform an output into an input, for use with an alt.  Consider a case where you
	*	have a process that wants to wait for either input on its "in" channel or output being taken on its
	*	"out" channel.  You could do the following (using int as the data type for illustration):
		@code
		One2OneChannel<int> c;
		One2OneChannel<bool> acks;
		AltChanin<bool> ackIn = acks.reader();
		ScopedForking forking;
		forking.forkInThisThread(new NotifySender<int>(c.reader(),out,acks.writer()));
		
		Alternative alt( boost::assign::list_of<Guard*>(in.inputGuard())(ackIn.inputGuard()) );
		while (true)
		{
			switch (alt.fairSelect())
			{
				case 0:
					{
						//input on in channel:
						int n;
						in >> n;
						// ... do something ...
						break;
					}
				case 1:
					{
						//output was taken:
						bool b;
						ackIn >> b;
						// ... do something ...
						break;
					}
			}
		}
		@endcode
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.	
	*/
	template <typename DATA_TYPE>
	class NotifySender : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out;
		Chanout<bool> sentAck;
		DATA_TYPE t;
	protected:
		void run()
		{
			try
			{
				while (true)
				{
					in >> t;
					out << t;
					sentAck << true;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
				sentAck.poison();
			}
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _in The channel to read the data from
		*	@param _out The channel to send the data out on
		*	@param _sentAck The channel to send the send-notifications on.
		*/
		inline NotifySender(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out, const Chanout<bool>& _sentAck)
			:	CSProcess(65536),in(_in),out(_out),sentAck(_sentAck)
		{
		}
	};	
	
	//TODO later document this process once I'm sure I'm happy with the design
	template <typename DATA_TYPE>
	class Delta : public CSProcess
	{
	private:
		Chanin<DATA_TYPE> in;
		Chanout<DATA_TYPE> out0;
		Chanout<DATA_TYPE> out1;
		
		One2OneChannel<DATA_TYPE> intermedChan0;
		One2OneChannel<DATA_TYPE> intermedChan1;
		
		Chanout<DATA_TYPE> intermedOut0;
		Chanout<DATA_TYPE> intermedOut1;
		
		One2OneChannel<bool> ackChan0;
		One2OneChannel<bool> ackChan1;
		
		Chanin<bool> ack0;
		Chanin<bool> ack1;
		
		DATA_TYPE t;
	protected:
		void run()
		{
			ScopedForking forking;
		
			try
			{
				forking.forkInThisThread(new NotifySender<DATA_TYPE>(intermedChan0.reader(),out0,ackChan0.writer()));
				forking.forkInThisThread(new NotifySender<DATA_TYPE>(intermedChan1.reader(),out1,ackChan1.writer()));
				
				intermedOut0 = intermedChan0.writer();
				intermedOut1 = intermedChan1.writer();
				ack0 = ackChan0.reader();
				ack1 = ackChan1.reader();
			
				while (true)
				{
					in >> t;
					intermedOut0 << t;
					intermedOut1 << t;
					bool b;
					ack0 >> b;
					ack1 >> b;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out0.poison();
				out1.poison();
			}
		}
	public:
		inline Delta(const Chanin<DATA_TYPE>& _in, const Chanout<DATA_TYPE>& _out0, const Chanout<DATA_TYPE>& _out1)
			:	CSProcess(65536),in(_in),out0(_out0),out1(_out1)
		{
		}
	};	
	
	/**
	*	Merges the data from multiple channels into one.
	*
	*	The Merger process waits for input on one of its channels, and then sends it out on its output
	*	channel.  The waiting for input is done fairly (using Alternative::fairSelect()) so starvation
	*	will not occur.
	*
	*	If you can alter that part of your application, you may want to consider using Any2OneChannel
	*	or BufferedAny2OneChannel.  The latter, with a FIFO buffer of size 1, has exactly the same	
	*	effect as multiple One2OneChannel joined together with this process.  That is, this:
		@code
		One2OneChannel<int> c,d,e;
		Run(InParallel
			(new Widget(c.writer()))
			(new Widget(d.writer()))
			(new Merger<int>(c.reader(),d.reader(),e.writer()))
			(new OtherWidget(e.reader()))
		);
		@endcode
	*	Is the same as:
		@code
		FIFOBuffer<int>::Factory factory(1);
		BufferedAny2OneChannel<int> c(factory);		
		Run(InParallel
			(new Widget(c.writer()))
			(new Widget(c.writer()))			
			(new OtherWidget(c.reader()))
		);
		@endcode
	*
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.	
	*/
	template <typename DATA_TYPE>
	class Merger : public CSProcess
	{
	private:
		DATA_TYPE t;
		std::vector< AltChanin<DATA_TYPE> > in;		
		Chanout<DATA_TYPE> out;
	protected:
		void run()
		{
			std::list<Guard*> guards;
			for (typename std::vector< AltChanin<DATA_TYPE> >::const_iterator it = in.begin();it != in.end();it++)
			{
				guards.push_back(it->inputGuard());
			}
			Alternative alt(guards);
			
			try
			{
				while (true)
				{
					unsigned int index = alt.fairSelect();
					in[index] >> t;				
					out << t;
				}
			}
			catch (PoisonException&)
			{
				for (typename std::vector< AltChanin<DATA_TYPE> >::const_iterator it = in.begin();it != in.end();it++)
				{
					it->poison();
				}
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process with two input channels (a constructor for convenience)
		*
		*	@param _in0 One of the input channels
		*	@param _in1 The other input channel
		*	@param _out The output channel
		*/
		inline Merger(const AltChanin<DATA_TYPE>& _in0, const AltChanin<DATA_TYPE>& _in1, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),out(_out)
		{
			in.push_back(_in0);
			in.push_back(_in1);
		}
		
		/**
		*	Constructs the process using the given std::vector of input channels
		*
		*	To use other collection classes, use the other constructor that takes iterators as parameters.
		*
		*	@param _in The vector of input channels
		*	@param _out The output channel
		*/
		inline Merger(const std::vector< AltChanin<DATA_TYPE> >& _in, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_in),out(_out)
		{
		}
		
		/**
		*	Constructs the process using input channels defined by an iterator range.
		*
		*	The iterators should be iterators over a collection of AltChanin<DATA_TYPE>.  They will
		*	be passed to the constructor of std::vector< AltChanin<DATA_TYPE> > so they must meet
		*	the requirements for that; all the values from *_begin up to (but not including) *_end
		*	will be added to the list of input channels
		*
		*	@param _begin The iterator of the first channel input-end to add
		*	@param _end The iterator beyond the last channel input-end to add
		*	@param _out The output channel
		*/
		template <typename ITERATOR>
		inline Merger(ITERATOR _begin, ITERATOR _end,const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_begin,_end),out(_out)
		{
		}
	};
	
	/**
	*	Merges the data from multiple channels into one, using extended input to prevent buffering.
	*
	*	The Merger process waits for input on one of its channels, and then sends it out on its output
	*	channel as part of an extended input (finishing the extended input once the output has
	*	completed).  The waiting for input is done fairly (using Alternative::fairSelect()) so starvation
	*	will not occur.
	*
	*	If you can alter that part of your application, you may want to consider using Any2OneChannel
	*	or BufferedAny2OneChannel.  The former has exactly the same	
	*	effect as multiple One2OneChannel joined together with this process.  That is, this:
		@code
		One2OneChannel<int> c,d,e;
		Run(InParallel
			(new Widget(c.writer()))
			(new Widget(d.writer()))
			(new ExtMerger<int>(c.reader(),d.reader(),e.writer()))
			(new OtherWidget(e.reader()))
		);
		@endcode
	*	Is the same as:
		@code		
		Any2OneChannel<int> c(factory);		
		Run(InParallel
			(new Widget(c.writer()))
			(new Widget(c.writer()))			
			(new OtherWidget(c.reader()))
		);
		@endcode
	*	
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a default constructor, and must meet the requirements of the channels it is being used with.	
	*/
	template <typename DATA_TYPE>
	class ExtMerger : public CSProcess
	{
	private:
		DATA_TYPE t;
		std::vector< AltChanin<DATA_TYPE> > in;		
		Chanout<DATA_TYPE> out;
	protected:
		void run()
		{
			std::list<Guard*> guards;
			for (typename std::vector< AltChanin<DATA_TYPE> >::const_iterator it = in.begin();it != in.end();it++)
			{
				guards.push_back(it->inputGuard());
			}
			Alternative alt(guards);
			
			try
			{
				while (true)
				{
					unsigned int index = alt.fairSelect();
					{
						ScopedExtInput<DATA_TYPE> ext(in[index],&t);
						out << t;
					}
				}
			}
			catch (PoisonException&)
			{
				for (typename std::vector< AltChanin<DATA_TYPE> >::const_iterator it = in.begin();it != in.end();it++)
				{
					it->poison();
				}
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process with two input channels (a constructor for convenience)
		*
		*	@param _in0 One of the input channels
		*	@param _in1 The other input channel
		*	@param _out The output channel
		*/
		inline ExtMerger(const AltChanin<DATA_TYPE>& _in0, const AltChanin<DATA_TYPE>& _in1, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),out(_out)
		{
			in.push_back(_in0);
			in.push_back(_in1);
		}
		
		/**
		*	Constructs the process using the given std::vector of input channels
		*
		*	To use other collection classes, use the other constructor that takes iterators as parameters.
		*
		*	@param _in The vector of input channels
		*	@param _out The output channel
		*/
		inline ExtMerger(const std::vector< AltChanin<DATA_TYPE> >& _in, const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_in),out(_out)
		{
		}
		
		/**
		*	Constructs the process using input channels defined by an iterator range.
		*
		*	The iterators should be iterators over a collection of AltChanin<DATA_TYPE>.  They will
		*	be passed to the constructor of std::vector< AltChanin<DATA_TYPE> > so they must meet
		*	the requirements for that; all the values from *_begin up to (but not including) *_end
		*	will be added to the list of input channels
		*
		*	@param _begin The iterator of the first channel input-end to add
		*	@param _end The iterator beyond the last channel input-end to add
		*	@param _out The output channel
		*/
		template <typename ITERATOR>
		inline ExtMerger(ITERATOR _begin, ITERATOR _end,const Chanout<DATA_TYPE>& _out)
			:	CSProcess(65536),in(_begin,_end),out(_out)
		{
		}
	};
	
	
	/**
	*	Evaluates a function and sends out the output
	*
	*	This process calls a function/calls operator() on a function class <i>once</i> and sends out the output.
	*	It can be viewed as a simple way to distribute some processing into another thread.
	*	For example, this will search a list in another process:
		@code
		template <typename T>
		class ListSearcher
		{
		public:
			std::list<T>* data;
			T const * target;
			typename std::list<T>::iterator operator()()
			{
				return std::find(data->begin(),data->end(),*target);
			}
			
			inline ListSearcher(std::list<T>* _data,T const * _target)
				:	data(_data),target(_target)
			{
			}
		};
		
		// In some process:
		list<int> hugeList;
		// ... fill hugeList ...
		One2OneChannel< list<int>::iterator > c0,c1,c2;
		const int target0 = 36;
		const int target1 = 49;
		const int target2 = 64;
		ScopedForking forking;
		forking.fork(new EvaluateFunction< list<int>::iterator , ListSearcher<int> >(ListSearcher<int>(&hugeList,&target0),c0.writer()));
		forking.fork(new EvaluateFunction< list<int>::iterator , ListSearcher<int> >(ListSearcher<int>(&hugeList,&target1),c1.writer()));
		forking.fork(new EvaluateFunction< list<int>::iterator , ListSearcher<int> >(ListSearcher<int>(&hugeList,&target2),c2.writer()));
		
		list<int>::iterator res0,res1,res2;
		c0.reader() >> res0;
		c1.reader() >> res1;
		c2.reader() >> res2;
		
		if (res0 == hugeList.end() && res1 == hugeList.end() && res2 == hugeList.end())
		{
			//No values found, do something
		}
		@endcode
	*
	*	A simpler example is:
		@code
		One2OneChannel<int> c;
		ScopedForking forking;
		forking.fork(new EvaluateFunction<int>(&::rand,c.writer()));
		int n;
		//Read a random value into n:
		c.reader() >> n;
		@endcode
	*
	*
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq RESULT Requirements
	*
	*	RESULT must meet the requirements of the channels it is being used with.
	*
	*	@section tempreq FUNCTION Requirements
	*
	*	FUNCTION must have a copy constructor, and must either be:
	*	- A pointer to a no-arg function that returns a value that can be cast into RESULT without a cast. 
	*	- A class that supports operator() that returns a value that can be cast into RESULT without a cast. 
	*/
	template <typename RESULT, typename FUNCTION = RESULT (*) ()>
	class EvaluateFunction : public CSProcess
	{
	private:
		FUNCTION func;
		Chanout<RESULT> out;		
	protected:
		void run()
		{
			try
			{
				out << func();
			}
			catch (PoisonException&)
			{
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _func The function to call
		*	@param _out The output channel to send the result out on.
		*/
		inline EvaluateFunction(const FUNCTION& _func,const Chanout<RESULT>& _out)
			:	func(_func),out(_out)
		{
		}
	};
	
	//TODO later document that func should take a Mobile<BarrierEnd>& (no const), and doesn't have to worry about enrolling/resigning
	template <typename RESULT, typename FUNCTION = RESULT (*) (Mobile<BarrierEnd>&)>
	class EvaluateFunctionBarrier : public CSProcess
	{
	private:
		FUNCTION func;
		Chanout<RESULT> out;
		Mobile<BarrierEnd> end;		
	protected:
		void run()
		{
			try
			{
				end->enroll();
				out << func(end);
				end->resign();
			}
			catch (PoisonException&)
			{
				out.poison();
			}
		}
	public:
		inline EvaluateFunctionBarrier(const FUNCTION& _func,const Chanout<RESULT>& _out,const Mobile<BarrierEnd>& _end)
			:	func(_func),out(_out),end(_end)
		{
		}
	};	
	
	/**
	*	This process acts like an Id process, but applies a transformation to the data before sending it on.
	*
	*	For example, the following code will make the process continually read in a value, and send out the cosine of that value:
		@code
		Run(new FunctionProcess<float,float>(in,out,&cosf));
		@endcode
	*
	*	Similarly, this code will continually read in a type MyVariant, and apply MyVisitor to it, returning the result:
		@code
		//class MyVisitor : static_visitor<std::string>
		// MyVisitor is assumed to have an operator()(const MyVariant& v) method 
		// that calls apply_visitor(*this,v);
		
		Run(new FunctionProcess<MyVariant,std::string,MyVisitor>(in,out, MyVisitor() ));		
		@endcode
	*
	*	If you want to send more than one parameter to the function consider using a
	*	data structure (either a class/struct, or std::pair or boost::tuple) and potentially wrapping a function (or function class)
	*	around your desired function that separates out these parameters and passes them to the real function.
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq DATA_TYPE_IN Requirements
	*
	*	DATA_TYPE_IN must have a default constructor, and must meet the requirements of the channels it is being used with.
	*
	*	@section tempreq2 DATA_TYPE_OUT Requirements
	*
	*	DATA_TYPE_OUT must meet the requirements of the channels it is being used with.
	*
	*	@section tempreq FUNCTION Requirements
	*
	*	FUNCTION must have a copy constructor, and must either be:
	*	- A pointer to function that takes one parameter (that DATA_TYPE_IN can be converted to without a cast) 
	*	and returns a value that can be cast into DATA_TYPE_OUT	without a cast. 
	*	- A class that supports operator() with one parameter (that DATA_TYPE_IN can be converted to without a cast) 
	*	and returns a value that can be cast into DATA_TYPE_OUT	without a cast. 
	*/
	template <typename DATA_TYPE_IN, typename DATA_TYPE_OUT,typename FUNCTION = DATA_TYPE_OUT (*)(const DATA_TYPE_IN&) >
	class FunctionProcess : public CSProcess
	{
	private:
		FUNCTION func;
		Chanin<DATA_TYPE_IN> in;
		Chanout<DATA_TYPE_OUT> out;
		DATA_TYPE_IN data;
	protected:
		void run()
		{
			try
			{
				while (true)
				{
					in >> data;
					out << func(data);
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _in The input channel
		*	@param _out The output channel
		*	@param _func The function to call
		*/
		inline FunctionProcess(const Chanin<DATA_TYPE_IN> _in,const Chanout<DATA_TYPE_OUT>& _out, const FUNCTION& _func)
			:	in(_in),out(_out),func(_func)
		{
		}
	};
	
	/**
	*	Poisons the end of a channel and then exits.
	*
	*	It is typically used in sequence with another process using the same channel, so that once the previous
	*	process has finished, a channel it uses will be poisoned.  For example:
		@code
		//Assuming out is some Chanout<int> we have:
		Run(InSequenceOneThread
			(new WriterProcess<int>(out,1,7) ) //Send the value "1" 7 times
			(new ChannelPoisoner< Chanout<int> >(out) ) //Then poison the channel
		);	
		@endcode
	*
	*	To use this process, you will need to #include <cppcsp/common/basic.h>
	*
	*	@section tempreq CHANNEL_END_TYPE Requirements
	*
	*	CHANNEL_END_TYPE must have a copy constructor, and must have a poison() method.  In practice, this means
	*	CHANNEL_END_TYPE will usually be either Chanin or Chanout (AltChanin is also possible, but would have the 
	*	same effect as using Chanin)
	*/
	template <typename CHANNEL_END_TYPE>
	class ChannelPoisoner : public csp::CSProcess
	{
	private:
		CHANNEL_END_TYPE end;
	protected:
		void run()
		{
			end.poison();
		}
	public:
		/**
		*	Constructs the process
		*
		*	@param _end The channel end to poison		
		*/
		inline ChannelPoisoner(const CHANNEL_END_TYPE& _end)
			:	CSProcess(65536),end(_end)
		{
		}
	};
	
	//TODO later move this to a new header file:
	
	template <typename CHANNEL_TYPE, typename PROCESS, typename PROCESS_DATA>
	class ConnectionHandler : public CSProcess
	{
	private:
		Chanin< Mobile< CHANNEL_TYPE > > in;
		std::list< Mobile< CHANNEL_TYPE > > channels;
		PROCESS_DATA data;
		bool forkAsThread;
	protected:
		void run()
		{
			ScopedForking forking;
			
			try
			{
				while (true)
				{
					Mobile< CHANNEL_TYPE > mob;
					in >> mob;
					CSProcessPtr proc = new PROCESS(mob->reader(),mob->writer(),data);
					channels.push_back(mob);
					if (forkAsThread)
					{
						forking.fork(proc);
					}
					else
					{
						forking.forkInThisThread(proc);
					}
				}
			}
			catch (PoisonException&)
			{
				in.poison();
			}
		}
	public:
		inline ConnectionHandler(const Chanin< Mobile< CHANNEL_TYPE > >& _in, const PROCESS_DATA& _data, bool _forkAsThread = true)
			:	in(_in),data(_data),forkAsThread(_forkAsThread)
		{
		}
	};
	
} //namespace common

} //namespace csp
