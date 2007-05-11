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

/** @file src/bucket.h
*	@brief Contains the header for the Bucket class
*/

class BarrierTest;

namespace csp
{
	/**
	*	A bucket is a synchronisation primitive.  Processes "fall into" a bucket and wait until the bucket is "flushed".
	*
	*	Unlike a barrier, which has a concept of enrollment and waits until all processes are ready to synchronise,
	*	any process can fall into a bucket, and they are all released when any process calls flush.
	*
	*	The use of buckets is also covered in the appropriate @ref tut-buckets "tutorial section on buckets".
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	Bucket is almost identical to its C++CSP v1.x version.  The only differences are that the return type
	*	of the flush method has changed, and the holding method has also changed (it is no longer const, and its
	*	return type has changed).  The semantics are the same as they ever were.
	*/
	class Bucket : private internal::Primitive, public boost::noncopyable
	{
	private:
		typedef std::pair<internal::ProcessPtr,internal::ProcessPtr> ProcessQueue;
		std::map<ThreadId,ProcessQueue> processes;
		volatile usign32 processCount;
		internal::PureSpinMutex mutex;			
	public:
		inline Bucket()
			:	processCount(0)
		{
		}
		
		/**
		*	Waits until the bucket is flushed.
		*
		*	Any process may fall into a bucket (i.e. no enrollment is needed).  It will then wait until another
		*	process calls the flush() function.
		*/
		void fallInto()
		{
			ThreadId id = currentThread();
			mutex.claim();
			
				std::map<ThreadId,ProcessQueue>::iterator it = processes.find(id);
				
				if (it == processes.end())
				{
					it = processes.insert(std::make_pair(id,std::make_pair(NullProcessPtr,NullProcessPtr))).first;
				}
							
				ProcessQueue& queue = it->second;
			
				addProcessToQueue(&(queue.first),&(queue.second),currentProcess());
				
				processCount++;
			
			mutex.release();
			
			reschedule();
		}
		
		/**
		*	Flushes the bucket, freeing all process that were waiting on it.
		*
		*	When flush() is called, all processes that have previously "fallen into" the bucket (since the last flush) are freed.  
		*	The bucket then becomes empty.
		*
		*	@section compat C++CSP v1.x Compatibility
		*
		*	In C++CSP v1.x the return type of this method was void.  This change should not break any existing code.
		*
		*	@return The number of processes that were freed by this flush.  It may be zero if no processes were waiting.
		*/
		usign32 flush()
		{
			usign32 ret;
			mutex.claim();
			
				ret = processCount;
				processCount = 0;
				
				for (std::map<ThreadId,ProcessQueue>::const_iterator it = processes.begin();it != processes.end();it++)
				{
					freeProcessChain(it->second.first,it->second.second);
				}
		
				processes.clear();
				
			mutex.release();
		
			return ret;
		}
		
		/**
		*	Gets the number of processes currently "in" (waiting on) the bucket.
		*		
		*	@section compat C++CSP v1.x Compatibility
		*
		*	This method used to be const and return int.  It now returns usign32 and is not const.  You may have to adjust
		*	existing code.
		*
		*	@return The number of processes currently "in" (i.e. waiting on) the bucket.
		*/
		usign32 holding()
		{
			usign32 ret;
			mutex.claim();
				ret = processCount;
			mutex.release();
			return ret;
		}
		
		//For testing:
		friend class ::BarrierTest;
	};
}


