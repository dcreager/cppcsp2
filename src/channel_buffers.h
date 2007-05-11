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
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/less_equal.hpp>
#include <boost/mpl/times.hpp>
#include <boost/pool/pool_alloc.hpp>

class AltChannelTest;

//Most of the documentation is at the bottom of this file

namespace csp
{			
	template <typename DATA_TYPE>
	class ChannelBuffer 
	{	
	public:
		/**
		*	Checks whether an input would succeed on the buffer.
		*
		*	Informally, inputWouldSucceed() will return true when the buffer is non-empty, but
		*	false when the buffer is empty.
		*
		*	Sub-classes will implement this method.
		*
		*	This method must return false at some point (i.e. the buffer must be emptyable).  Because
		*	poison from writer to reader is only noticed when this method returns false, this method
		*	must at some point return false to allow a writer's poison to transmit properly.
		*
		*	This restriction should not be a problem for most buffers (intuitively, buffers should be
		*	empty once all data has been read from them).  The sort of buffer that you should avoid
		*	implementing is one where data is (for example) forever overwritten but never actually removed
		*	from the buffer (so that a reader can always read the most recent value, over and over again).
		*	To produce the same effect, use a process that holds the value, with request/reply channels for
		*	accessing the value.
		*/	
		virtual bool inputWouldSucceed() = 0;
		
		/**
		*	Checks whether an output (of a specific item of data) would succeed on the buffer.
		*
		*	Informally, outputWouldSucceed() will return true when the buffer is not full, but
		*	false when the buffer is full.  
		*
		*	Passing the item of data allows buffers that may depend on the particular item given
		*	(particularly buffers that in some way aggregate the buffered data, or deal with
		*	items that can have different sizes).
		*
		*	Sub-classes will implement this method.
		*
		*	source must not be NULL.
		*/
		virtual bool outputWouldSucceed(const DATA_TYPE* source) = 0;	
		
		/**
		*	Adds a single item of data to the buffer.
		*
		*	Sub-classes will implement this method.
		*
		*	@pre this method will not be called when outputWouldSucceed(source) returns false
		*
		*	@param source A pointer to the source data to be added to the buffer.  The data
		*	pointed to by source will be copied from, and the pointer does not have to remain
		*	valid after this call has returned.  source must not be NULL.
		*/
		virtual void put(const DATA_TYPE* source) = 0;

		/**
		*	Gets a single item of data from the buffer.  The item of data is copied into dest and
		*	then removed from the buffer
		*
		*	Sub-classes will implement this method.
		*
		*	@pre this method will not be called when inputWouldSucceed() returns false
		*
		*	@param dest A pointer to the destination to copy the data into.  dest must not be NULL.
		*/
		virtual void get(DATA_TYPE* dest) = 0;		
		
		/**
		*	Starts an extended input from the buffer.
		*
		*	Sub-classes should implement this method.
		*
		*	The implementation of this method for a new buffer may not be obvious.  Put simply,
		*	when a channel begins an extended input on a buffered channel, it calls this beginExtGet method
		*	on the buffer.  When the channel finishes the extended input, it calls endExtGet on the
		*	buffer.  Each beginExtGet call will be followed by an extEndGet before the next beginEndGet
		*	call.  How you implement these methods will require thinking about how the semantics
		*	of an extended input on your buffer.
		*
		*	Examples of semantic implementations can be found in the buffers provided with C++CSP2.  See
		*	the @ref channelbuffers "Channel Buffers" page for more information.
		*
		*	@pre this method will not be called when inputWouldSucceed() returns false
		*
		*	@param dest A pointer to the destination to copy the data into.  dest must not be NULL.
		*/
		virtual void beginExtGet(DATA_TYPE* dest) = 0;
		
		/**
		*	Ends the current extended input from the buffer.
		*
		*	Sub-classes should implement this method.
		*
		*	The implementation of this method for a new buffer may not be obvious.  Put simply,
		*	when a channel begins an extended input on a buffered channel, it calls this beginExtGet method
		*	on the buffer.  When the channel finishes the extended input, it calls endExtGet on the
		*	buffer.  Each beginExtGet call will be followed by an extEndGet before the next beginEndGet
		*	call.  How you implement these methods will require thinking about how the semantics
		*	of an extended input on your buffer.
		*
		*	endExtGet will always be called either:
		*	-	Following a call to beginExtGet()
		*	-	Following a call to beginExtGet() and a subsequent call to clear()
		*
		*	In particular, the buffer may be empty when this method is called, so remember to check for that.
		*
		*	Examples of semantic implementations can be found in the buffers provided with C++CSP2.  See
		*	the @ref channelbuffers "Channel Buffers" page for more information.
		*/
		virtual void endExtGet() = 0;
		
		/**
		*	Clears all data from the buffer
		*
		*	@post inputWouldSucceed() will return false
		*/
		virtual void clear() = 0;
		
		virtual ~ChannelBuffer()
		{
		}

	}; //class ChannelBuffer

	
	template <typename DATA_TYPE>
	class ChannelBufferFactory
	{
	public:
		/**
		*	Creates a new channel buffer.
		*
		*	The returned channel buffer will be deleted by the channel using it, so
		*	it should be allocated using new (that is, allocated on the heap), and a
		*	pointer to it should not need to be retained by the factory.
		*
		*	Note that the method is const, this is important to remember when implementing a channel factory.
		*
		*	@return A new channel buffer
		*/
		virtual ChannelBuffer<DATA_TYPE>* createBuffer() const = 0;
		
		virtual ~ChannelBufferFactory()
		{
		}
	};
	

	template <typename DATA_TYPE,typename BUFFER_TYPE>
	class ChannelBufferFactoryImpl : public ChannelBufferFactory<DATA_TYPE>
	{		
	public:
		/**
		*	Returns a new instance of BUFFER_TYPE using its default constructor
		*/
		virtual ChannelBuffer<DATA_TYPE>* createBuffer() const
		{
			return new BUFFER_TYPE ();
		}		
	};
	

	template <typename DATA_TYPE,typename BUFFER_TYPE>
	class SizedChannelBufferFactoryImpl : public ChannelBufferFactory<DATA_TYPE>
	{
		unsigned size;
	public:
		/**
		*	Returns a new instance of BUFFER_TYPE using a constructor that takes an integer size argument
		*/
		virtual ChannelBuffer<DATA_TYPE>* createBuffer() const
		{
			return new BUFFER_TYPE (size);
		}
		
		/**
		*	Creates a buffer factory that will create buffers of the specified size
		*
		*	@param _size The size argument to pass to the constructor of new instances of BUFFER_TYPE
		*/
		inline SizedChannelBufferFactoryImpl(unsigned _size)
			:	size(_size)
		{
		}
	};
	
	template <typename DATA_TYPE>
	class InfiniteFIFOBuffer : public ChannelBuffer<DATA_TYPE>, public boost::noncopyable
	{
	private:
		std::list<DATA_TYPE> buffer;	
	public:
		virtual bool inputWouldSucceed()
		{
			return (false == buffer.empty());
		}
		virtual bool outputWouldSucceed(const DATA_TYPE*)
		{
			return true;
		}
	
		virtual void get(DATA_TYPE* dest)
		{
			//No point duplicating code:
			InfiniteFIFOBuffer::beginExtGet(dest);
			InfiniteFIFOBuffer::endExtGet();
		}
		
		virtual void beginExtGet(DATA_TYPE* dest)
		{
			*dest = buffer.front();
		}
		
		virtual void endExtGet()
		{
			if (false == buffer.empty())
				buffer.pop_front();			
		}

		virtual void put(const DATA_TYPE* source)
		{
			buffer.push_back(*source);			
		}
		
		virtual void clear()
		{
			buffer.clear();
		}
		
	public:
		/**
		*	A channel buffer factory that can be used with this buffer.
		*/
		typedef ChannelBufferFactoryImpl<DATA_TYPE,InfiniteFIFOBuffer<DATA_TYPE> > Factory;		
	};
		
	template <typename DATA_TYPE, typename _LIST_DATA_TYPE = 
		typename boost::mpl::if_< 
			//If sizeof(DATA_TYPE) <= 4 * sizeof(void*)
			boost::mpl::less_equal<  
				boost::mpl::sizeof_<DATA_TYPE>,
				boost::mpl::times< boost::mpl::int_<4>, boost::mpl::sizeof_<void*> >
			>,
			//Then use a pool allocator:
			std::list< DATA_TYPE,boost::fast_pool_allocator<DATA_TYPE> >,
			//Else use a normal list:
			std::list< DATA_TYPE >
		>::type
	>
	class FIFOBuffer : public ChannelBuffer<DATA_TYPE>, public boost::noncopyable
	{
	private:
		typedef _LIST_DATA_TYPE LIST_DATA_TYPE;
		LIST_DATA_TYPE buffer;
		const unsigned maxSize;
	public:		
		virtual bool inputWouldSucceed()
		{
			return (false == buffer.empty());
		}
		virtual bool outputWouldSucceed(const DATA_TYPE*)
		{
			return ((buffer.size() + 1) <= maxSize);
		}		
	
		virtual void put(const DATA_TYPE* source)
		{
			buffer.push_back(*source);
		}

		virtual void get(DATA_TYPE* dest)
		{
			//No point duplicating code:
			FIFOBuffer::beginExtGet(dest);
			FIFOBuffer::endExtGet();
		}
		
		virtual void beginExtGet(DATA_TYPE* dest)
		{
			*dest = buffer.front();
		}
		
		virtual void endExtGet()
		{			
			if (false == buffer.empty())
				buffer.pop_front();			
		}
		
		virtual void clear()
		{
			buffer.clear();
		}
	
		/**
		*	A channel buffer factory that can be used with this buffer.
		*/
		typedef SizedChannelBufferFactoryImpl<DATA_TYPE,FIFOBuffer<DATA_TYPE,LIST_DATA_TYPE> > Factory;
	
		/**
		*	Constructor
		*
		*	@param n The size of the FIFO buffer to create
		*/
		inline explicit FIFOBuffer(unsigned int n)
			:	maxSize(n)
		{
		}
		
		//For testing:
		friend class ::AltChannelTest;
	};
	
	template <typename DATA_TYPE, typename _LIST_DATA_TYPE = 
		typename boost::mpl::if_< 
			//If sizeof(DATA_TYPE) <= 4 * sizeof(void*)
			boost::mpl::less_equal<  
				boost::mpl::sizeof_<DATA_TYPE>,
				boost::mpl::times< boost::mpl::int_<4>, boost::mpl::sizeof_<void*> >
			>,
			//Then use a pool allocator:
			std::list< DATA_TYPE,boost::fast_pool_allocator<DATA_TYPE> >,
			//Else use a normal list:
			std::list< DATA_TYPE >
		>::type
	>
	class OverwritingBuffer : public ChannelBuffer<DATA_TYPE>, public boost::noncopyable
	{
	private:
		typedef _LIST_DATA_TYPE LIST_DATA_TYPE;
		LIST_DATA_TYPE buffer;
		const unsigned maxSize;
		bool shouldRemove;
	public:		
		virtual bool inputWouldSucceed()
		{
			return (false == buffer.empty());
		}
		virtual bool outputWouldSucceed(const DATA_TYPE*)
		{
			return true;
		}		
	
		virtual void put(const DATA_TYPE* source)
		{
			if (buffer.size() == maxSize)
			{
				shouldRemove = false;
				buffer.pop_front();
			}
						
			buffer.push_back(*source);			
		}

		virtual void get(DATA_TYPE* dest)
		{
			//No point duplicating code:
			*dest = buffer.front();
			buffer.pop_front();
		}
		
		virtual void beginExtGet(DATA_TYPE* dest)
		{
			shouldRemove = true;
			*dest = buffer.front();
		}
		
		virtual void endExtGet()
		{	
			if (shouldRemove && false == buffer.empty())
			{
				buffer.pop_front();
			}
		}
		
		virtual void clear()
		{
			buffer.clear();
		}
	
		/**
		*	A channel buffer factory that can be used with this buffer.
		*/
		typedef SizedChannelBufferFactoryImpl<DATA_TYPE,OverwritingBuffer<DATA_TYPE,LIST_DATA_TYPE> > Factory;
	
		/**
		*	Constructor
		*
		*	@param n The size of the overwriting buffer to create
		*/
		inline explicit OverwritingBuffer(unsigned int n)
			:	maxSize(n)
		{
		}
		
		//For testing:
		friend class ::AltChannelTest;
	};
	
	
	/**
	*	@defgroup channelbuffers Channel Buffers
	*
	*	Channel Buffers are for use with buffered channels such as BufferedOne2OneChannel.  A channel buffer
	*	is a sub-class of ChannelBuffer, and provides a buffering strategy for a buffered channel.
	*
	*	Currently three buffers are provided with the library: limited-size FIFO buffering (FIFOBuffer), 
	*	unlimited-size FIFO buffering (InfiniteFIFOBuffer) and limited-size overwriting FIFO buffering
	*	(OverwritingBuffer).
	*
	*	Channel buffers are provided to buffered channels via channel buffer factories.  This ensures that
	*	the same channel buffer is not accidentally used with multiple channels, as each buffered
	*	channel asks the factory for a separate buffer.  A channel buffer factory is a sub-class
	*	of ChannelBufferFactory.
	*
	*	<b>C++CSP v1.x Compatibility:</b>
	*
	*	ZeroBuffer has been removed in C++CSP2.  ZeroBuffer did not
	*	actually have correct semantics, and so has been removed.  If you wish to change between buffered
	*	and unbuffered channels easily, consider using a ChannelFactory.  
	*
	*	In C++CSP v1.x, channel buffers provided the clone method to ensure that the same buffer was
	*	not used by two different channels.  This has been replaced by channel buffer factories in C++CSP2,
	*	that remove the need to create a fully-fledged buffer just to pass to buffered channels to be
	*	cloned.  The channel factories are a lower-overhead cleaner way of accomplishing the same thing.
	*
	*	@{
	*/
	
	/** @class ChannelBuffer	
	*	The base class for channel buffers.
	*	
	*	Channel buffers should implement the four pure virtual buffers documented below; get(),put(),beginExtGet(),endExtGet()
	*
	*	Sub-classes of ChannelBuffer usually have a Factory typedef in them
	*	that represents the ChannelBufferFactory implementation for that buffer.  This
	*	allows channel buffers to be easily used as template arguments.
	*
	*	For more information, see the page on @ref channelbuffers "Channel Buffers" and the 
	*	section in the guide on @ref tut-buffered "Buffered Channels".
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	ChannelBuffer itself does not place any restrictions on DATA_TYPE.  However, its implementations (its sub-classes)
	*	will do.
	*/
	
	/** @class ChannelBufferFactory
	*
	*	The base class for all the channel buffer factories.  Use this class if you want
	*	your process to use a channel buffer factory provided from outside your process,
	*	for example:
	*	@code
		class BufferedPingPong : public CSProcess
		{
		private:
			ChannelBufferFactory<DATA_TYPE>* factory;
		protected:
			void run()
			{
				BufferedOne2OneChannel c(factory), d(factory);
				
				Run(InParallel
					(new Prefix<int>(d.reader(),c.writer(),0))
					(new Id<int>(c.reader(),d.writer())
				);
			}
		public:
			inline explicit BufferedPingPong(ChannelBufferFactory<DATA_TYPE>* _factory)
				:	factory(_factory)
			{
			}
		};
		@endcode
	*
	*	If you create a new ChannelBuffer, you will need to implement a factory for it.
	*	To do this, you may find the ChannelBufferFactoryImpl and 
	*	SizedChannelBufferFactoryImpl classes useful
	*
	*	For more information, see the page on @ref channelbuffers "Channel Buffers" and the 
	*	section in the guide on @ref tut-buffered "Buffered Channels".
	*/
			
	/** @class FIFOBuffer
	*	A FIFO buffer with a fixed maximum capacity.
	*
	*	Unlike InfiniteFIFOBuffer, FIFOBuffer has a fixed maximum size.  Data items come out of the buffer
	*	in the same order that they went in.
	*
	*	The complicated-looking second template parameter
	*	is a bit of C++ trickery (with the help of boost) that you are unlikely to need to ever alter.  It implements
	*	a heuristic that uses std::list with a standard allocator (using new and delete) if sizeof(DATA_TYPE) is strictly greater
	*	than four times the sizeof(void*).  Otherwise, boost's pool allocator is used to speed up the continual re-allocation
	*	of the linked list nodes.  If you are <i>incredibly</i> concerned with memory or speed usage you may want to play with
	*	this parameter, but otherwise leave it as the default.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor and support assignment.
	*/
	
	/** @class InfiniteFIFOBuffer	
	*	A FIFO buffer of unlimited capacity.
	*
	*	This is a FIFO (First-In First-Out) buffer that can grow to any size.  Be very careful when using this buffer,
	*	as a channel can inadvertently use up as much memory as your system allows, especially if the receiver deadlocks
	*	or otherwise unexpectedly terminates.  It is advised that you use a FIFOBuffer with a high capacity instead, but 
	*	sometimes an infinite buffer can be useful.  
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor and support assignment.
	*/
	
	/** @class OverwritingBuffer
	*	An overwriting FIFO buffer.
	*
	*	When this buffer is not full, it acts exactly like a standard FIFO buffer (i.e. csp::FIFOBuffer).  
	*	However, if a process attempts to write to this buffer when it is full, they will begin overwriting
	*	the oldest data in the buffer.  Thus, writes to the buffer will never block.  When a reader reads
	*	from the buffer, they will always read the oldest item of data (i.e. the oldest item that has not yet
	*	been overwritten).
	*
	*	For an explanation of the second template parameter, see the documentation for csp::FIFOBuffer.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE must have a copy constructor and support assignment.
	*/

	/** @class ChannelBufferFactoryImpl
	*
	*	A default implementation of ChannelBufferFactory for use with buffers that have default constructors
	*
	*	@see InfiniteFIFOBuffer::Factory
	*/
	
	/** @class SizedChannelBufferFactoryImpl
	*
	*	A default implementation of ChannelBufferFactory for use with buffers that have constructors that take a size argument
	*
	*	@see FIFOBuffer::Factory
	*/
	
	/** @} */ //end of group	
	
	
	
	//NOTE: sending empty collections into an aggregating buffer has no effect!
	//Aggregating buffers are undocumented for now, and should not be used
	
	//TODO later this buffer will forever block if you try to put into it more than it can hold at one time
	template <typename DATA_TYPE>
	class PrimitiveAggregatingFIFOBuffer : public ChannelBuffer< std::vector<DATA_TYPE> >
	{
	private:
		std::vector<DATA_TYPE> buffer;				
		const size_t limit;
	public:		
		inline explicit PrimitiveAggregatingFIFOBuffer(const size_t _limit)
			:	limit(_limit)
		{
			buffer.reserve(limit);
		}
		
		virtual bool inputWouldSucceed()
		{
			return (false == buffer.empty());
		}
		virtual bool outputWouldSucceed(const std::vector<DATA_TYPE>* source)
		{
			return (buffer.size() + source->size()) <= limit;
		}
	
		virtual void put(const std::vector<DATA_TYPE>* source)
		{
			const size_t prevSize = buffer.size();
			buffer.resize(prevSize + source->size());
			memcpy(&(buffer[prevSize]),&(source[0]),sizeof(DATA_TYPE) * source->size());
		}		
		
		virtual void get(std::vector<DATA_TYPE>* dest)
		{
			dest->swap(buffer);
			buffer.reserve(limit);
			buffer.resize(0);
		}
				
		virtual void beginExtGet(std::vector<DATA_TYPE>* dest)
		{
			get(dest);
		}
				
		virtual void endExtGet()
		{
		}
		
		virtual void clear()
		{
			buffer.clear();
		}
		
		typedef SizedChannelBufferFactoryImpl< std::vector<unsigned char> , PrimitiveAggregatingFIFOBuffer<unsigned char> > Factory;
	};
	
	template <typename DATA_TYPE>
	class AggregatingFIFOBuffer : public ChannelBuffer< Mobile< std::list<DATA_TYPE> > >
	{
	private:
		typedef Mobile< std::list<DATA_TYPE> > MOBILE;
	
		Mobile< std::list<DATA_TYPE> > buffer;
		//list::size() can be O(N) on some implementations, so we'll keep track:
		size_t curSize;
		const size_t limit;
		
		
	public:
		virtual bool inputWouldSucceed()
		{
			return (buffer && (false == buffer.empty()));
		}
		virtual bool outputWouldSucceed(const MOBILE* source)
		{
			return (curSize + (*source)->size() < limit);
		}
	
		virtual void put(const Mobile<std::list<DATA_TYPE> >* source)
		{
			if (buffer && curSize > 0)
			{
				curSize += (*source)->size();
				buffer->splice(buffer->end(),**source);
				source->blank();
			}
			else
			{
				buffer = *source;
				curSize = buffer->size();
			}
			
		}		
		
		virtual void get(MOBILE* dest)
		{
			*dest = buffer;
			curSize = 0;
		}
				
		virtual void beginExtGet(MOBILE* dest)
		{
			get(dest);
		}
				
		virtual void endExtGet()
		{
		}
		
		virtual void clear()
		{
			buffer.blank();
		}
	};
	
		
	
	

} //namespace csp
